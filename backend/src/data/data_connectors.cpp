#include "induspilot/data/data_connectors.hpp"

#include <chrono>
#include <cerrno>
#include <string>
#include <utility>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace induspilot::data {
namespace {

struct Endpoint {
    std::string host;
    int port{0};
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

bool tcpReachable(const Endpoint& endpoint, int timeoutMs) {
    if (endpoint.host.empty() || endpoint.port <= 0) {
        return false;
    }

#ifdef _WIN32
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        return false;
    }
#endif

    addrinfo hints{};
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;

    addrinfo* result = nullptr;
    const auto port = std::to_string(endpoint.port);
    if (getaddrinfo(endpoint.host.c_str(), port.c_str(), &hints, &result) != 0) {
#ifdef _WIN32
        WSACleanup();
#endif
        return false;
    }

    bool connected = false;
    for (auto* item = result; item != nullptr; item = item->ai_next) {
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
            }
        }
        close(socketHandle);
#endif
        if (connected) {
            break;
        }
    }

    freeaddrinfo(result);
#ifdef _WIN32
    WSACleanup();
#endif
    return connected;
}

}  // namespace

DataConnectors::DataConnectors(app::AppConfig config) : config_(std::move(config)) {}

DependencyRequirements DataConnectors::requirements() const {
    return DependencyRequirements{
        config_.storage.repositoryStore == "mysql",
        config_.redis.sessionStore == "redis",
        false,
        config_.ai.enabled && config_.ai.provider == "http",
    };
}

DependencyStatus DataConnectors::probe() const {
    const auto required = requirements();
    const auto mysqlEndpoint = config_.mysql.uri.empty()
        ? endpointFromHostPort(config_.mysql.host, config_.mysql.port)
        : endpointFromUri(config_.mysql.uri, config_.mysql.port > 0 ? config_.mysql.port : 3306);
    const auto redisEndpoint = endpointFromUri(config_.redis.uri, config_.redis.port);
    const auto aiEndpoint = endpointFromUri(config_.ai.endpoint, 80);

    const auto mysqlAvailable = required.mysql && tcpReachable(mysqlEndpoint, config_.readiness.probeTimeoutMs);
    const auto redisAvailable = required.redis && tcpReachable(redisEndpoint, config_.readiness.probeTimeoutMs);
    const auto aiAvailable = required.ai && tcpReachable(aiEndpoint, config_.readiness.probeTimeoutMs);

    return DependencyStatus{
        {required.mysql, required.mysql ? mysqlAvailable : true,
         required.mysql ? (mysqlAvailable ? "TCP endpoint reachable" : "TCP endpoint unavailable")
                        : "not required by repository_store"},
        {required.redis, required.redis ? redisAvailable : true,
         required.redis ? (redisAvailable ? "TCP endpoint reachable" : "TCP endpoint unavailable")
                        : "not required by session_store"},
        {false, true, "optional dependency is not probed"},
        {false, required.ai ? aiAvailable : true,
         required.ai ? (aiAvailable ? "TCP endpoint reachable (optional)" : "TCP endpoint unavailable (optional)")
                     : "disabled"},
    };
}

std::string DataConnectors::describe() const {
    return "MySQL, Redis and enabled HTTP AI probes use configured TCP endpoints; MongoDB is optional and not probed";
}

}  // namespace induspilot::data
