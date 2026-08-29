#include "induspilot/data/data_connectors.hpp"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <future>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace induspilot::data {
namespace {

using Clock = std::chrono::steady_clock;
using Deadline = Clock::time_point;

struct Endpoint {
    std::string host;
    int port{0};
};

struct ProbeResult {
    bool available{false};
    std::string reason;
};

std::string stripScheme(const std::string& value) {
    const auto pos = value.find("://");
    if (pos == std::string::npos) {
        return value;
    }
    return value.substr(pos + 3);
}

Endpoint endpointFromUri(const std::string& uri, int defaultPort) {
    auto value = stripScheme(uri);
    const auto pathPos = value.find('/');
    if (pathPos != std::string::npos) {
        value = value.substr(0, pathPos);
    }
    const auto atPos = value.rfind('@');
    if (atPos != std::string::npos) {
        value = value.substr(atPos + 1);
    }

    Endpoint endpoint;
    const auto colonPos = value.rfind(':');
    if (colonPos == std::string::npos) {
        endpoint.host = value;
        endpoint.port = defaultPort;
        return endpoint;
    }

    endpoint.host = value.substr(0, colonPos);
    try {
        endpoint.port = std::stoi(value.substr(colonPos + 1));
    } catch (...) {
        endpoint.port = defaultPort;
    }
    return endpoint;
}

Endpoint endpointFromHostPort(const std::string& host, int port) {
    return Endpoint{host, port};
}

int remainingMilliseconds(const Deadline deadline) {
    const auto now = Clock::now();
    if (now >= deadline) {
        return 0;
    }
    const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count();
    return static_cast<int>(std::clamp<long long>(remaining, 1, (std::numeric_limits<int>::max)()));
}

#ifdef _WIN32
bool ensureWinsock() {
    static std::once_flag flag;
    static bool initialized = false;
    std::call_once(flag, [] {
        WSADATA data{};
        initialized = WSAStartup(MAKEWORD(2, 2), &data) == 0;
    });
    return initialized;
}
#endif

struct ResolveRequest {
    std::mutex mutex;
    std::condition_variable condition;
    addrinfo* result{nullptr};
    int error{0};
    bool done{false};
    bool abandoned{false};
};

class DnsResolver {
public:
    DnsResolver() : worker_([this] { run(); }) {}

    bool resolve(const Endpoint& endpoint, const Deadline deadline, addrinfo*& result, int& error) {
        if (remainingMilliseconds(deadline) == 0) {
            return false;
        }

        const auto request = std::make_shared<ResolveRequest>();
        {
            std::lock_guard lock(mutex_);
            if (requests_.size() >= 32) {
                return false;
            }
            requests_.push_back({request, endpoint.host, std::to_string(endpoint.port)});
        }
        condition_.notify_one();

        std::unique_lock lock(request->mutex);
        if (!request->condition.wait_until(lock, deadline, [&request] { return request->done; })) {
            request->abandoned = true;
            return false;
        }
        result = request->result;
        request->result = nullptr;
        error = request->error;
        return error == 0 && result != nullptr;
    }

private:
    struct WorkItem {
        std::shared_ptr<ResolveRequest> request;
        std::string host;
        std::string service;
    };

    void run() {
        for (;;) {
            WorkItem work;
            {
                std::unique_lock lock(mutex_);
                condition_.wait(lock, [this] { return !requests_.empty(); });
                work = std::move(requests_.front());
                requests_.pop_front();
            }

            addrinfo* resolved = nullptr;
            addrinfo hints{};
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_family = AF_UNSPEC;
            const auto resolveError = getaddrinfo(work.host.c_str(), work.service.c_str(), &hints, &resolved);
            std::unique_lock lock(work.request->mutex);
            if (work.request->abandoned) {
                lock.unlock();
                if (resolved != nullptr) {
                    freeaddrinfo(resolved);
                }
                continue;
            }
            work.request->result = resolved;
            work.request->error = resolveError;
            work.request->done = true;
            lock.unlock();
            work.request->condition.notify_one();
        }
    }

    std::mutex mutex_;
    std::condition_variable condition_;
    std::deque<WorkItem> requests_;
    std::thread worker_;
};

DnsResolver& dnsResolver() {
    // The resolver intentionally lives for the process lifetime because getaddrinfo has no portable cancellation API.
    static auto* resolver = new DnsResolver();
    return *resolver;
}

bool resolveWithDeadline(
    const Endpoint& endpoint,
    const Deadline deadline,
    addrinfo*& result,
    int& error) {
    addrinfo numericHints{};
    numericHints.ai_socktype = SOCK_STREAM;
    numericHints.ai_family = AF_UNSPEC;
    numericHints.ai_flags = AI_NUMERICHOST;
    if (getaddrinfo(endpoint.host.c_str(), std::to_string(endpoint.port).c_str(), &numericHints, &result) == 0) {
        error = 0;
        return true;
    }
    return dnsResolver().resolve(endpoint, deadline, result, error);
}

ProbeResult tcpProbe(const Endpoint& endpoint, const Deadline deadline) {
    if (endpoint.host.empty() || endpoint.port <= 0 || endpoint.port > 65535) {
        return {false, "TCP endpoint configuration is invalid"};
    }

#ifdef _WIN32
    if (!ensureWinsock()) {
        return {false, "Winsock initialization failed"};
    }
#endif

    addrinfo* result = nullptr;
    int resolveError = 0;
    if (!resolveWithDeadline(endpoint, deadline, result, resolveError)) {
        return {false, remainingMilliseconds(deadline) == 0 ? "DNS resolution timed out" : "DNS resolution failed"};
    }

    bool timedOut = false;
    bool connected = false;
    for (auto* item = result; item != nullptr; item = item->ai_next) {
        const auto timeoutMs = remainingMilliseconds(deadline);
        if (timeoutMs == 0) {
            timedOut = true;
            break;
        }

        const auto socketHandle = socket(item->ai_family, item->ai_socktype, item->ai_protocol);
#ifdef _WIN32
        if (socketHandle == INVALID_SOCKET) {
            continue;
        }
        u_long nonBlocking = 1;
        if (ioctlsocket(socketHandle, FIONBIO, &nonBlocking) != 0) {
            closesocket(socketHandle);
            continue;
        }
        const auto connectResult = connect(socketHandle, item->ai_addr, static_cast<int>(item->ai_addrlen));
        if (connectResult == 0) {
            connected = true;
        } else {
            const auto errorCode = WSAGetLastError();
            if (errorCode == WSAEWOULDBLOCK || errorCode == WSAEINPROGRESS) {
                fd_set writable;
                FD_ZERO(&writable);
                FD_SET(socketHandle, &writable);
                timeval timeout{timeoutMs / 1000, (timeoutMs % 1000) * 1000};
                if (select(0, nullptr, &writable, nullptr, &timeout) > 0) {
                    int error = 0;
                    int errorSize = sizeof(error);
                    getsockopt(socketHandle, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &errorSize);
                    connected = error == 0;
                } else if (remainingMilliseconds(deadline) == 0) {
                    timedOut = true;
                }
            }
        }
        closesocket(socketHandle);
#else
        if (socketHandle < 0) {
            continue;
        }
        const auto flags = fcntl(socketHandle, F_GETFL, 0);
        if (flags < 0 || fcntl(socketHandle, F_SETFL, flags | O_NONBLOCK) < 0) {
            close(socketHandle);
            continue;
        }
        const auto connectResult = connect(socketHandle, item->ai_addr, item->ai_addrlen);
        if (connectResult == 0) {
            connected = true;
        } else if (errno == EINPROGRESS) {
            fd_set writable;
            FD_ZERO(&writable);
            FD_SET(socketHandle, &writable);
            timeval timeout{timeoutMs / 1000, (timeoutMs % 1000) * 1000};
            if (select(socketHandle + 1, nullptr, &writable, nullptr, &timeout) > 0) {
                int error = 0;
                socklen_t errorSize = sizeof(error);
                getsockopt(socketHandle, SOL_SOCKET, SO_ERROR, &error, &errorSize);
                connected = error == 0;
            } else if (remainingMilliseconds(deadline) == 0) {
                timedOut = true;
            }
        }
        close(socketHandle);
#endif
        if (connected) {
            break;
        }
    }

    freeaddrinfo(result);
    if (connected) {
        return {true, "TCP endpoint reachable"};
    }
    if (timedOut || remainingMilliseconds(deadline) == 0) {
        return {false, "TCP probe timed out"};
    }
    return {false, "TCP endpoint unavailable"};
}

}  // namespace

DataConnectors::DataConnectors(app::AppConfig config) : config_(std::move(config)) {}

DependencyRequirements DataConnectors::requirements() const {
    return DependencyRequirements{
        config_.storage.repositoryStore == "mysql",
        config_.redis.sessionStore == "redis",
        config_.storage.aiInteractionStore == "mongodb",
        config_.ai.enabled && config_.ai.provider == "http",
        config_.ai.required,
    };
}

DependencyStatus DataConnectors::probe() const {
    const auto required = requirements();
    const auto mysqlEndpoint = config_.mysql.uri.empty()
        ? endpointFromHostPort(config_.mysql.host, config_.mysql.port)
        : endpointFromUri(config_.mysql.uri, config_.mysql.port > 0 ? config_.mysql.port : 3306);
    const auto redisEndpoint = endpointFromUri(config_.redis.uri, config_.redis.port);
    const auto aiEndpoint = endpointFromUri(config_.ai.endpoint, 80);
    const auto deadline = Clock::now() + std::chrono::milliseconds(config_.readiness.probeTimeoutMs);

    std::future<ProbeResult> mysqlFuture;
    std::future<ProbeResult> redisFuture;
    std::future<ProbeResult> aiFuture;
    std::future<ProbeResult> mongodbFuture;
    if (required.mysql) {
        mysqlFuture = std::async(std::launch::async, [mysqlEndpoint, deadline] {
            return tcpProbe(mysqlEndpoint, deadline);
        });
    }
    if (required.redis) {
        redisFuture = std::async(std::launch::async, [redisEndpoint, deadline] {
            return tcpProbe(redisEndpoint, deadline);
        });
    }
    if (required.ai) {
        aiFuture = std::async(std::launch::async, [aiEndpoint, deadline] {
            return tcpProbe(aiEndpoint, deadline);
        });
    }
    if (required.mongodb) {
        const auto mongodbEndpoint = config_.mongodb.uri.empty()
            ? endpointFromHostPort(config_.mongodb.host, config_.mongodb.port)
            : endpointFromUri(config_.mongodb.uri, config_.mongodb.port > 0 ? config_.mongodb.port : 27017);
        mongodbFuture = std::async(std::launch::async, [mongodbEndpoint, deadline] {
            return tcpProbe(mongodbEndpoint, deadline);
        });
    }

    ProbeResult mysqlResult{true, "not required by repository_store"};
    ProbeResult redisResult{true, "not required by session_store"};
    ProbeResult aiResult{true, "disabled"};
    ProbeResult mongodbResult{true, "not required by ai_interaction_store"};
    if (required.mysql) {
        mysqlResult = mysqlFuture.get();
    }
    if (required.redis) {
        redisResult = redisFuture.get();
    }
    if (required.ai) {
        aiResult = aiFuture.get();
    }
    if (required.mongodb) {
        mongodbResult = mongodbFuture.get();
    }

    return DependencyStatus{
        {required.mysql, mysqlResult.available, mysqlResult.reason, required.mysql},
        {required.redis, redisResult.available, redisResult.reason, required.redis},
        {required.mongodb, mongodbResult.available, mongodbResult.reason, required.mongodb},
        {required.aiRequired, aiResult.available, required.ai ? aiResult.reason : "disabled", required.ai},
    };
}

std::string DataConnectors::describe() const {
    return "MySQL, Redis, selected MongoDB AI storage and enabled HTTP AI probes use configured TCP endpoints";
}

}  // namespace induspilot::data
