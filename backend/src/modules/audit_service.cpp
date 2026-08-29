#include "induspilot/modules/audit_service.hpp"

#include "induspilot/data/in_memory_repositories.hpp"
#include "induspilot/modules/password_hasher.hpp"

#include <algorithm>
#include <atomic>
#include <charconv>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <exception>
#include <iomanip>
#include <iterator>
#include <optional>
#include <sstream>
#include <thread>
#include <utility>

#ifdef INDUSPILOT_WITH_DROGON
#include <drogon/drogon.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#endif
#endif

namespace induspilot::modules {
namespace {

std::string currentTimestamp() {
    const auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &time);
#else
    localtime_r(&time, &localTime);
#endif
    std::ostringstream out;
    out << std::put_time(&localTime, "%Y-%m-%dT%H:%M:%S");
    return out.str();
}

std::string timestampFor(const std::chrono::system_clock::time_point& point) {
    const auto time = std::chrono::system_clock::to_time_t(point);
    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &time);
#else
    localtime_r(&time, &localTime);
#endif
    std::ostringstream out;
    out << std::put_time(&localTime, "%Y-%m-%dT%H:%M:%S");
    return out.str();
}

std::string nextAuditId() {
    static std::atomic<unsigned long long> sequence{0};
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const auto epochMillis = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    return "audit-" + std::to_string(epochMillis) + "-" + std::to_string(sequence.fetch_add(1) + 1);
}

std::int64_t nowUnixMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string deliveryWorkerToken() {
    static std::atomic<unsigned long long> sequence{0};
    return "audit-siem-" + std::to_string(nowUnixMs()) + "-" + std::to_string(sequence.fetch_add(1) + 1);
}

std::string boundedError(const std::string& value) {
    constexpr std::size_t kMaximumLength = 480;
    if (value.size() <= kMaximumLength) {
        return value;
    }
    return value.substr(0, kMaximumLength);
}

std::string canonicalAuditPayload(const domain::OperationAuditEvent& event, const std::string& previousHash) {
    std::ostringstream out;
    out << event.id << '\n'
        << event.actor << '\n'
        << event.action << '\n'
        << event.resourceType << '\n'
        << event.resourceId << '\n'
        << event.result << '\n'
        << event.traceId << '\n'
        << event.occurredAt << '\n'
        << previousHash;
    return out.str();
}

std::string calculateAuditHash(const domain::OperationAuditEvent& event, const std::string& previousHash) {
    return sha256Hex(canonicalAuditPayload(event, previousHash));
}
bool matches(const domain::OperationAuditEvent& event, const OperationAuditQuery& query) {
    return (!query.actor || event.actor == *query.actor) &&
        (!query.action || event.action == *query.action) &&
        (!query.resourceType || event.resourceType == *query.resourceType) &&
        (!query.result || event.result == *query.result) &&
        (!query.occurredFrom || event.occurredAt >= *query.occurredFrom) &&
        (!query.occurredTo || event.occurredAt <= *query.occurredTo);
}

#ifdef INDUSPILOT_WITH_DROGON
struct WebhookEndpoint {
    std::string path{"/"};
    std::string host;
    std::string port;
    std::uint16_t portNumber{0};
    bool https{false};
    bool valid{false};
};

WebhookEndpoint parseWebhookEndpoint(const std::string& target) {
    const auto schemePosition = target.find("://");
    if (schemePosition == std::string::npos) {
        return {};
    }
    const auto scheme = target.substr(0, schemePosition);
    if (scheme != "http" && scheme != "https") {
        return {};
    }
    const auto pathPosition = target.find_first_of("/?#", schemePosition + 3);
    const auto authorityEnd = pathPosition == std::string::npos ? target.size() : pathPosition;
    const auto authority = target.substr(schemePosition + 3, authorityEnd - schemePosition - 3);
    if (authority.empty() || authority.find('@') != std::string::npos) {
        return {};
    }
    WebhookEndpoint endpoint;
    endpoint.valid = true;
    endpoint.https = scheme == "https";
    if (authority.front() == '[') {
        const auto closingBracket = authority.find(']');
        if (closingBracket == std::string::npos || closingBracket == 1) {
            return {};
        }
        endpoint.host = authority.substr(1, closingBracket - 1);
        if (closingBracket + 1 < authority.size()) {
            if (authority[closingBracket + 1] != ':') {
                return {};
            }
            endpoint.port = authority.substr(closingBracket + 2);
        }
    } else {
        const auto firstColon = authority.find(':');
        if (firstColon != authority.rfind(':')) {
            return {};
        }
        endpoint.host = authority.substr(0, firstColon == std::string::npos ? authority.size() : firstColon);
        if (firstColon != std::string::npos) {
            endpoint.port = authority.substr(firstColon + 1);
        }
    }
    if (endpoint.host.empty() || (!endpoint.port.empty() && endpoint.port.find_first_not_of("0123456789") != std::string::npos)) {
        return {};
    }
    if (endpoint.port.empty()) {
        endpoint.port = endpoint.https ? "443" : "80";
    }
    unsigned int parsedPort = 0;
    const auto portResult = std::from_chars(endpoint.port.data(), endpoint.port.data() + endpoint.port.size(), parsedPort);
    if (portResult.ec != std::errc{} || portResult.ptr != endpoint.port.data() + endpoint.port.size() || parsedPort == 0 || parsedPort > 65535) {
        return {};
    }
    endpoint.portNumber = static_cast<std::uint16_t>(parsedPort);
    if (pathPosition != std::string::npos) {
        endpoint.path = target.substr(pathPosition);
    }
    return endpoint;
}

bool forbiddenIpv4(const unsigned char* bytes) {
    return bytes[0] == 0 || bytes[0] == 10 || bytes[0] == 127 ||
        (bytes[0] == 100 && bytes[1] >= 64 && bytes[1] <= 127) ||
        (bytes[0] == 169 && bytes[1] == 254) ||
        (bytes[0] == 172 && bytes[1] >= 16 && bytes[1] <= 31) ||
        (bytes[0] == 192 && (bytes[1] == 0 || bytes[1] == 2 || bytes[1] == 168)) ||
        (bytes[0] == 192 && bytes[1] == 88 && bytes[2] == 99) ||
        (bytes[0] == 198 && (bytes[1] == 18 || bytes[1] == 19 || bytes[1] == 51)) ||
        (bytes[0] == 203 && bytes[1] == 0 && bytes[2] == 113) || bytes[0] >= 224;
}

bool forbiddenIpv6(const unsigned char* bytes) {
    bool allZero = true;
    bool loopback = true;
    for (int index = 0; index < 16; ++index) {
        allZero = allZero && bytes[index] == 0;
        if (index < 15) {
            loopback = loopback && bytes[index] == 0;
        }
    }
    loopback = loopback && bytes[15] == 1;
    if (allZero || loopback || (bytes[0] & 0xfe) == 0xfc || (bytes[0] & 0xc0) == 0x80 || bytes[0] == 0xff ||
        (bytes[0] == 0x20 && bytes[1] == 0x01 && bytes[2] == 0x0d && bytes[3] == 0xb8)) {
        return true;
    }
    if (std::memcmp(bytes, "\0\0\0\0\0\0\0\0\0\0\xff\xff", 12) == 0) {
        return forbiddenIpv4(bytes + 12);
    }
    return false;
}

std::optional<std::string> resolvePublicAddress(const WebhookEndpoint& endpoint) {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* addresses = nullptr;
    if (getaddrinfo(endpoint.host.c_str(), endpoint.port.c_str(), &hints, &addresses) != 0 || addresses == nullptr) {
        if (addresses != nullptr) {
            freeaddrinfo(addresses);
        }
        return std::nullopt;
    }
    bool hasAddress = false;
    bool allPublic = true;
    std::string selected;
    for (auto* address = addresses; address != nullptr; address = address->ai_next) {
        if (address->ai_family == AF_INET) {
            hasAddress = true;
            const auto* value = reinterpret_cast<const sockaddr_in*>(address->ai_addr);
            const auto forbidden = forbiddenIpv4(reinterpret_cast<const unsigned char*>(&value->sin_addr));
            allPublic = allPublic && !forbidden;
            if (selected.empty() && !forbidden) {
                char buffer[INET_ADDRSTRLEN]{};
                if (inet_ntop(AF_INET, &value->sin_addr, buffer, sizeof(buffer)) != nullptr) {
                    selected = buffer;
                }
            }
        } else if (address->ai_family == AF_INET6) {
            hasAddress = true;
            const auto* value = reinterpret_cast<const sockaddr_in6*>(address->ai_addr);
            const auto forbidden = forbiddenIpv6(reinterpret_cast<const unsigned char*>(&value->sin6_addr));
            allPublic = allPublic && !forbidden;
            if (selected.empty() && !forbidden) {
                char buffer[INET6_ADDRSTRLEN]{};
                if (inet_ntop(AF_INET6, &value->sin6_addr, buffer, sizeof(buffer)) != nullptr) {
                    selected = buffer;
                }
            }
        } else {
            allPublic = false;
        }
    }
    freeaddrinfo(addresses);
    if (!hasAddress || !allPublic || selected.empty()) {
        return std::nullopt;
    }
    return selected;
}

bool hostAllowed(const std::string& host, const std::string& configuredHosts) {
    std::size_t start = 0;
    while (start <= configuredHosts.size()) {
        const auto end = configuredHosts.find(',', start);
        auto candidate = configuredHosts.substr(start, end == std::string::npos ? configuredHosts.size() - start : end - start);
        const auto first = candidate.find_first_not_of(" \t");
        const auto last = candidate.find_last_not_of(" \t");
        if (first != std::string::npos) {
            candidate = candidate.substr(first, last - first + 1);
            if (candidate.size() == host.size()) {
                bool equal = true;
                for (std::size_t index = 0; index < host.size(); ++index) {
                    equal = equal && std::tolower(static_cast<unsigned char>(candidate[index])) == std::tolower(static_cast<unsigned char>(host[index]));
                }
                if (equal) {
                    return true;
                }
            }
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }
    return false;
}
#endif

class DefaultAuditDeliverySink final : public AuditDeliverySink {
public:
    explicit DefaultAuditDeliverySink(app::AuditConfig config) : config_(std::move(config)) {}

    AuditDeliveryResult deliver(const domain::OperationAuditEvent& event) const override {
#ifdef INDUSPILOT_WITH_DROGON
        const auto endpoint = parseWebhookEndpoint(config_.siemWebhookUrl);
        if (!endpoint.valid || !hostAllowed(endpoint.host, config_.siemWebhookAllowedHosts)) {
            return {false, "SIEM webhook target 无效或不在允许列表中"};
        }
        const auto address = resolvePublicAddress(endpoint);
        if (!address.has_value()) {
            return {false, "SIEM webhook target 未解析为公网地址"};
        }
        try {
            Json::Value payload;
            payload["id"] = event.id;
            payload["actor"] = event.actor;
            payload["action"] = event.action;
            payload["resourceType"] = event.resourceType;
            payload["resourceId"] = event.resourceId;
            payload["result"] = event.result;
            payload["traceId"] = event.traceId;
            payload["occurredAt"] = event.occurredAt;
            payload["previousHash"] = event.previousHash;
            payload["eventHash"] = event.eventHash;
            auto request = drogon::HttpRequest::newHttpJsonRequest(payload);
            request->setMethod(drogon::Post);
            request->setPath(endpoint.path);
            auto hostHeader = endpoint.host.find(':') == std::string::npos ? endpoint.host : "[" + endpoint.host + "]";
            if (endpoint.portNumber != (endpoint.https ? 443 : 80)) {
                hostHeader += ":" + endpoint.port;
            }
            request->addHeader("Host", hostHeader);
            const auto client = drogon::HttpClient::newHttpClient(*address, endpoint.portNumber, endpoint.https, nullptr, false, true);
            const auto response = client->sendRequest(request, static_cast<double>((std::max)(config_.siemWebhookTimeoutMs, 1)) / 1000.0);
            if (response.first != drogon::ReqResult::Ok || !response.second) {
                return {false, "SIEM webhook 请求失败"};
            }
            const auto statusCode = static_cast<int>(response.second->statusCode());
            if (statusCode < 200 || statusCode >= 300) {
                return {false, "SIEM webhook 返回状态码 " + std::to_string(statusCode)};
            }
            return {true, {}};
        } catch (const std::exception& ex) {
            return {false, "SIEM webhook 异常：" + boundedError(ex.what())};
        }
#else
        (void)event;
        return {false, "SIEM webhook 需要启用 Drogon HTTP 传输"};
#endif
    }

private:
    app::AuditConfig config_;
};

}  // namespace

AuditService::AuditService() : AuditService(std::make_shared<data::InMemoryOperationAuditRepository>()) {}

AuditService::AuditService(std::shared_ptr<data::OperationAuditRepository> repository,
    std::shared_ptr<AuditDeliverySink> deliverySink,
    app::AuditConfig config,
    std::shared_ptr<data::AuditDeliveryQueueRepository> deliveryQueue,
    std::shared_ptr<MetricsRegistry> metrics)
    : repository_(std::move(repository)),
      deliverySink_(std::move(deliverySink)),
      config_(std::move(config)),
      deliveryQueue_(std::move(deliveryQueue)),
      metrics_(std::move(metrics)) {
    if (!repository_) {
        repository_ = std::make_shared<data::InMemoryOperationAuditRepository>();
    }
    if (deliverySink_ && !deliveryQueue_) {
        deliveryQueue_ = std::make_shared<data::InMemoryAuditDeliveryQueueRepository>();
    }
    if (deliverySink_ && deliveryQueue_) {
        deliveryWorker_ = std::thread(&AuditService::deliveryLoop, this);
    }
}

AuditService::~AuditService() {
    {
        std::lock_guard<std::mutex> lock(deliveryMutex_);
        stopDeliveryWorker_ = true;
    }
    deliveryWakeup_.notify_all();
    if (deliveryWorker_.joinable()) {
        deliveryWorker_.join();
    }
}

ServiceStatus AuditService::status() const {
    return ServiceStatus{"operation-audit", true, "operation audit repository is ready"};
}

domain::OperationAuditEvent AuditService::record(domain::OperationAuditEvent event) {
    domain::OperationAuditEvent saved;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (event.id.empty()) {
            event.id = nextAuditId();
        }
        if (event.occurredAt.empty()) {
            event.occurredAt = currentTimestamp();
        }
        if (event.result.empty()) {
            event.result = "success";
        }
        const auto latestEvent = repository_->latest();
        event.previousHash = latestEvent ? latestEvent->eventHash : "genesis";
        event.eventHash = calculateAuditHash(event, event.previousHash);
        saved = repository_->save(std::move(event));
    }
    if (deliverySink_ && deliveryQueue_) {
        try {
            deliveryQueue_->enqueue(saved, config_.siemWebhookMaxAttempts);
            recordDeliveryQueueDepths();
            deliveryWakeup_.notify_one();
        } catch (const std::exception&) {
            // Queue failures must not affect the durable audit record.
        }
    }
    return saved;
}

std::vector<domain::OperationAuditEvent> AuditService::events(const OperationAuditQuery& query) const {
    std::vector<domain::OperationAuditEvent> result;
    auto effectiveQuery = query;
    if (config_.retentionDays > 0) {
        const auto cutoff = timestampFor(std::chrono::system_clock::now() - std::chrono::hours(24 * config_.retentionDays));
        if (!effectiveQuery.occurredFrom || *effectiveQuery.occurredFrom < cutoff) {
            effectiveQuery.occurredFrom = cutoff;
        }
    }
    const auto events = repository_->list();
    std::copy_if(events.begin(), events.end(), std::back_inserter(result), [&effectiveQuery](const auto& event) { return matches(event, effectiveQuery); });
    return result;
}

std::vector<domain::OperationAuditEvent> AuditService::archiveEvents(const OperationAuditQuery& query) const {
    auto events = repository_->listForIntegrity();
    std::reverse(events.begin(), events.end());
    std::vector<domain::OperationAuditEvent> result;
    std::copy_if(events.begin(), events.end(), std::back_inserter(result), [&query](const auto& event) { return matches(event, query); });
    return result;
}

OperationAuditIntegrityReport AuditService::integrityReport() const {
    const auto events = repository_->listForIntegrity();
    OperationAuditIntegrityReport report;
    report.total = events.size();
    std::string previousHash = "genesis";
    for (const auto& event : events) {
        if (event.previousHash != previousHash || event.eventHash != calculateAuditHash(event, event.previousHash)) {
            report.verified = false;
            report.brokenEventId = event.id;
            report.latestHash = previousHash;
            return report;
        }
        previousHash = event.eventHash;
    }
    report.latestHash = previousHash;
    return report;
}

void AuditService::deliveryLoop() {
    while (true) {
        processDeliveryQueue();
        std::unique_lock<std::mutex> lock(deliveryMutex_);
        const auto pollInterval = std::chrono::milliseconds((std::max)(config_.siemWebhookPollMs, 10));
        deliveryWakeup_.wait_for(lock, pollInterval, [this] { return stopDeliveryWorker_; });
        if (stopDeliveryWorker_) {
            return;
        }
    }
}

void AuditService::processDeliveryQueue() {
    if (!deliverySink_ || !deliveryQueue_) {
        return;
    }
    try {
        const auto now = nowUnixMs();
        auto deliveries = deliveryQueue_->claimDue(now, now + 30000, 100, deliveryWorkerToken());
        for (auto& delivery : deliveries) {
            ++delivery.attemptCount;
            delivery.leaseUntilUnixMs = 0;
            delivery.leaseToken.clear();
            AuditDeliveryResult result;
            try {
                result = deliverySink_->deliver(delivery.event);
            } catch (const std::exception& ex) {
                result = {false, "SIEM webhook 异常：" + boundedError(ex.what())};
            } catch (...) {
                result = {false, "SIEM webhook 发生未知异常"};
            }
            if (result.delivered) {
                delivery.status = "sent";
                delivery.lastError.clear();
                delivery.deliveredAt = currentTimestamp();
                delivery.nextAttemptAtUnixMs = 0;
            } else if (delivery.attemptCount >= delivery.maxAttempts) {
                delivery.status = "dead_letter";
                delivery.lastError = boundedError(result.error.empty() ? "SIEM webhook 投递失败" : result.error);
                delivery.deliveredAt.clear();
                delivery.nextAttemptAtUnixMs = 0;
            } else {
                delivery.status = "retrying";
                delivery.lastError = boundedError(result.error.empty() ? "SIEM webhook 投递失败" : result.error);
                delivery.deliveredAt.clear();
                const auto exponent = (std::min)(delivery.attemptCount - 1, 10);
                delivery.nextAttemptAtUnixMs = nowUnixMs() + (1LL << exponent) * 1000;
            }
            const auto outcome = result.delivered ? "sent" :
                (delivery.attemptCount >= delivery.maxAttempts ? "dead_letter" : "retrying");
            deliveryQueue_->save(std::move(delivery));
            if (metrics_) {
                metrics_->recordNotificationDelivery("siem", outcome);
            }
        }
        recordDeliveryQueueDepths();
    } catch (const std::exception&) {
        // A queue or database failure is isolated from audit event recording and retried on the next poll.
    }
}

void AuditService::recordDeliveryQueueDepths() const {
    if (!deliveryQueue_ || !metrics_) {
        return;
    }
    const auto depths = deliveryQueue_->depths();
    metrics_->recordAuditSiemDeliveryQueueDepths(
        AuditSiemDeliveryQueueSnapshot{depths.queued, depths.retrying, depths.deadLetter});
}

std::shared_ptr<AuditDeliverySink> makeAuditDeliverySink(const app::AuditConfig& config) {
    if (!config.siemWebhookEnabled) {
        return nullptr;
    }
    return std::make_shared<DefaultAuditDeliverySink>(config);
}
}  // namespace induspilot::modules
