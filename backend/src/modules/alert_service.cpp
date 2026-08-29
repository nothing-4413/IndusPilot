#include "induspilot/modules/alert_service.hpp"

#include "induspilot/data/in_memory_repositories.hpp"

#ifdef INDUSPILOT_WITH_DROGON
#include <drogon/drogon.h>
#endif

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <random>
#include <sstream>
#include <cctype>
#include <utility>

namespace induspilot::modules {
namespace {

std::string currentTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
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

std::int64_t nowUnixMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::string workerToken() {
    static thread_local std::mt19937_64 generator(std::random_device{}());
    return std::to_string(generator()) + std::to_string(generator());
}

int severityRank(const std::string& severity) {
    if (severity == "critical") {
        return 3;
    }
    if (severity == "warning") {
        return 2;
    }
    if (severity == "info") {
        return 1;
    }
    return 0;
}

bool matchesRule(const domain::Alert& alert, const domain::AlertRule& rule) {
    if (!rule.enabled) {
        return false;
    }
    if (!rule.assetId.empty() && alert.assetId != rule.assetId) {
        return false;
    }
    return severityRank(alertSeverityToString(alert.severity)) >= severityRank(rule.minSeverity);
}

bool matches(const domain::Alert& alert, const AlertQuery& query) {
    if (query.assetId && alert.assetId != *query.assetId) {
        return false;
    }
    if (query.severity && alert.severity != *query.severity) {
        return false;
    }
    if (query.state && alert.state != *query.state) {
        return false;
    }
    return true;
}

std::string boundedError(std::string error) {
    if (error.size() > 255) {
        error.resize(255);
    }
    return error;
}

#ifdef INDUSPILOT_WITH_DROGON
struct WebhookEndpoint {
    std::string baseUrl;
    std::string path{"/"};
    std::string host;
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
    const auto pathPosition = target.find('/', schemePosition + 3);
    const auto authorityEnd = pathPosition == std::string::npos ? target.size() : pathPosition;
    const auto authority = target.substr(schemePosition + 3, authorityEnd - schemePosition - 3);
    const auto portPosition = authority.find(':');
    WebhookEndpoint endpoint;
    endpoint.valid = true;
    endpoint.host = authority.substr(0, portPosition == std::string::npos ? authority.size() : portPosition);
    if (endpoint.host.empty()) {
        return {};
    }
    if (pathPosition == std::string::npos) {
        endpoint.baseUrl = target;
    } else {
        endpoint.baseUrl = target.substr(0, pathPosition);
        endpoint.path = target.substr(pathPosition);
    }
    return endpoint;
}

bool webhookHostAllowed(const std::string& host, const std::string& configuredHosts) {
    std::size_t start = 0;
    while (start <= configuredHosts.size()) {
        const auto end = configuredHosts.find(',', start);
        const auto length = end == std::string::npos ? configuredHosts.size() - start : end - start;
        auto candidate = configuredHosts.substr(start, length);
        const auto first = candidate.find_first_not_of(" \t");
        const auto last = candidate.find_last_not_of(" \t");
        if (first != std::string::npos) {
            candidate = candidate.substr(first, last - first + 1);
            if (candidate.size() == host.size()) {
                bool equal = true;
                for (std::size_t index = 0; index < host.size(); ++index) {
                    if (std::tolower(static_cast<unsigned char>(candidate[index])) !=
                        std::tolower(static_cast<unsigned char>(host[index]))) {
                        equal = false;
                        break;
                    }
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

class DefaultAlertNotificationSender final : public AlertNotificationSender {
public:
    explicit DefaultAlertNotificationSender(app::NotificationConfig config)
        : config_(std::move(config)) {}

    NotificationDeliveryResult send(const domain::AlertNotification& notification) const override {
        if (notification.channel == "console") {
            return {true, {}};
        }
        if (notification.channel == "email") {
            return {false, "email 通知通道尚未配置邮件适配器"};
        }
        if (notification.channel != "webhook") {
            return {false, "不支持的通知通道：" + boundedError(notification.channel)};
        }
        if (!config_.webhookEnabled) {
            return {false, "webhook 通知通道未显式启用"};
        }
#ifndef INDUSPILOT_WITH_DROGON
        return {false, "webhook 通知需要启用 Drogon HTTP 传输"};
#else
        const auto endpoint = parseWebhookEndpoint(notification.target);
        if (!endpoint.valid) {
            return {false, "webhook target 必须是 HTTP(S) URL"};
        }
        if (!webhookHostAllowed(endpoint.host, config_.webhookAllowedHosts)) {
            return {false, "webhook target host 不在允许列表中"};
        }
        try {
            Json::Value payload;
            payload["notificationId"] = notification.id;
            payload["alertId"] = notification.alertId;
            payload["ruleId"] = notification.ruleId;
            payload["message"] = notification.message;
            payload["attemptCount"] = notification.attemptCount;
            auto request = drogon::HttpRequest::newHttpJsonRequest(payload);
            request->setMethod(drogon::Post);
            request->setPath(endpoint.path);
            auto client = drogon::HttpClient::newHttpClient(endpoint.baseUrl);
            const auto response = client->sendRequest(
                request, static_cast<double>((std::max)(config_.webhookTimeoutMs, 1)) / 1000.0);
            if (response.first != drogon::ReqResult::Ok || !response.second) {
                return {false, "webhook 请求失败"};
            }
            const auto statusCode = static_cast<int>(response.second->statusCode());
            if (statusCode < 200 || statusCode >= 300) {
                return {false, "webhook 返回状态码 " + std::to_string(statusCode)};
            }
            return {true, {}};
        } catch (const std::exception& ex) {
            return {false, "webhook 异常：" + boundedError(ex.what())};
        }
#endif
    }

private:
    app::NotificationConfig config_;
};

}  // namespace

std::optional<domain::AlertSeverity> alertSeverityFromString(const std::string& value) {
    if (value == "info") {
        return domain::AlertSeverity::Info;
    }
    if (value == "warning") {
        return domain::AlertSeverity::Warning;
    }
    if (value == "critical") {
        return domain::AlertSeverity::Critical;
    }
    return std::nullopt;
}

std::optional<domain::AlertState> alertStateFromString(const std::string& value) {
    if (value == "open") {
        return domain::AlertState::Open;
    }
    if (value == "acknowledged") {
        return domain::AlertState::Acknowledged;
    }
    if (value == "assigned") {
        return domain::AlertState::Assigned;
    }
    if (value == "resolved") {
        return domain::AlertState::Resolved;
    }
    if (value == "closed") {
        return domain::AlertState::Closed;
    }
    return std::nullopt;
}

std::string alertSeverityToString(domain::AlertSeverity severity) {
    switch (severity) {
        case domain::AlertSeverity::Info:
            return "info";
        case domain::AlertSeverity::Warning:
            return "warning";
        case domain::AlertSeverity::Critical:
            return "critical";
    }
    return "unknown";
}

std::string alertStateToString(domain::AlertState state) {
    switch (state) {
        case domain::AlertState::Open:
            return "open";
        case domain::AlertState::Acknowledged:
            return "acknowledged";
        case domain::AlertState::Assigned:
            return "assigned";
        case domain::AlertState::Resolved:
            return "resolved";
        case domain::AlertState::Closed:
            return "closed";
    }
    return "unknown";
}

AlertService::AlertService() : AlertService(std::make_shared<data::InMemoryAlertRepository>()) {}

std::shared_ptr<AlertNotificationSender> makeAlertNotificationSender(const app::NotificationConfig& config) {
    return std::make_shared<DefaultAlertNotificationSender>(config);
}

AlertService::AlertService(std::shared_ptr<data::AlertRepository> repository, std::shared_ptr<AlertNotificationSender> sender, std::shared_ptr<MetricsRegistry> metrics)
    : repository_(std::move(repository)), sender_(std::move(sender)), metrics_(std::move(metrics)) {
    if (!repository_) {
        repository_ = std::make_shared<data::InMemoryAlertRepository>();
    }
    if (!sender_) {
        sender_ = makeAlertNotificationSender(app::NotificationConfig{});
    }
}

ServiceStatus AlertService::status() const {
    return ServiceStatus{"alert-management", true, "alert repository is ready"};
}

domain::Alert AlertService::create(domain::Alert alert) {
    auto saved = repository_->save(std::move(alert));
    createNotificationsFor(saved);
    return saved;
}

std::optional<domain::Alert> AlertService::findById(const std::string& id) const {
    return repository_->findById(id);
}

std::vector<domain::Alert> AlertService::list(const AlertQuery& query) const {
    std::vector<domain::Alert> result;
    for (const auto& alert : repository_->list()) {
        if (matches(alert, query)) {
            result.push_back(alert);
        }
    }
    return result;
}

domain::AlertRule AlertService::createRule(domain::AlertRule rule) {
    return repository_->saveRule(std::move(rule));
}

std::vector<domain::AlertRule> AlertService::rules() const {
    return repository_->listRules();
}

std::vector<domain::AlertNotification> AlertService::notifications() const {
    return repository_->listNotifications();
}

NotificationDispatchSummary AlertService::dispatchQueuedNotifications() {
    NotificationDispatchSummary summary;
    const auto now = nowUnixMs();
    const auto token = workerToken();
    const auto claimed = repository_->claimDueNotifications(now, now + 30000, 100, token);
    for (auto notification : claimed) {
        notification = deliverNotification(std::move(notification));
        if (notification.status == "sent") {
            ++summary.sent;
        } else if (notification.status == "failed" || notification.status == "retrying" ||
                   notification.status == "dead_letter") {
            ++summary.failed;
        } else {
            ++summary.skipped;
        }
    }
    summary.skipped = (std::max)(0, static_cast<int>(repository_->listNotifications().size()) -
                                     static_cast<int>(claimed.size()));
    return summary;
}

std::optional<domain::AlertNotification> AlertService::retryNotification(const std::string& id) {
    for (auto notification : repository_->listNotifications()) {
        if (notification.id != id) {
            continue;
        }
        if (notification.status == "sent") {
            return notification;
        }
        notification.status = "retrying";
        notification.lastError.clear();
        notification.nextAttemptAtUnixMs = nowUnixMs();
        notification.leaseUntilUnixMs = 0;
        notification.leaseToken.clear();
        notification = repository_->saveNotification(std::move(notification));
        return deliverNotification(std::move(notification));
    }
    return std::nullopt;
}

void AlertService::createNotificationsFor(const domain::Alert& alert) {
    for (const auto& rule : repository_->listRules()) {
        if (!matchesRule(alert, rule)) {
            continue;
        }
        repository_->saveNotification(domain::AlertNotification{
            "notice-" + alert.id + "-" + rule.id,
            alert.id,
            rule.id,
            rule.channel,
            rule.target,
            "queued",
            "告警 " + alert.id + " 命中规则 " + rule.name,
            0,
            "",
            ""});
    }
}


domain::AlertNotification AlertService::deliverNotification(domain::AlertNotification notification) {
    notification.attemptCount += 1;
    notification.leaseUntilUnixMs = 0;
    notification.leaseToken.clear();
    if (notification.channel.empty() || notification.target.empty()) {
        notification.status = "failed";
        notification.lastError = "通知通道和目标不能为空";
        notification.deliveredAt.clear();
        notification.nextAttemptAtUnixMs = 0;
        if (notification.attemptCount < notification.maxAttempts) {
            notification.status = "retrying";
            notification.nextAttemptAtUnixMs = nowUnixMs() + (1LL << (std::min)(notification.attemptCount - 1, 10)) * 1000;
        } else {
            notification.status = "dead_letter";
        }
        if (metrics_) {
            metrics_->recordNotificationDelivery(notification.channel, notification.status);
        }
        return repository_->saveNotification(std::move(notification));
    }
    const auto delivery = sender_->send(notification);
    if (!delivery.delivered) {
        notification.status = "failed";
        notification.lastError = boundedError(delivery.error);
        notification.deliveredAt.clear();
        notification.nextAttemptAtUnixMs = 0;
        if (notification.attemptCount < notification.maxAttempts) {
            notification.status = "retrying";
            notification.nextAttemptAtUnixMs = nowUnixMs() + (1LL << (std::min)(notification.attemptCount - 1, 10)) * 1000;
        } else {
            notification.status = "dead_letter";
        }
        if (metrics_) {
            metrics_->recordNotificationDelivery(notification.channel, notification.status);
        }
        return repository_->saveNotification(std::move(notification));
    }

    notification.status = "sent";
    notification.lastError.clear();
    notification.deliveredAt = currentTimestamp();
    notification.nextAttemptAtUnixMs = 0;
    if (metrics_) {
        metrics_->recordNotificationDelivery(notification.channel, notification.status);
    }
    return repository_->saveNotification(std::move(notification));
}

std::optional<domain::Alert> AlertService::acknowledge(const std::string& id, const std::string& operatorId) {
    auto alert = repository_->findById(id);
    if (!alert) {
        return std::nullopt;
    }
    alert->state = domain::AlertState::Acknowledged;
    alert->acknowledgedBy = operatorId;
    return repository_->save(*alert);
}

std::optional<domain::Alert> AlertService::assign(const std::string& id, const std::string& assignee) {
    auto alert = repository_->findById(id);
    if (!alert) {
        return std::nullopt;
    }
    alert->state = domain::AlertState::Assigned;
    alert->assignedTo = assignee;
    return repository_->save(*alert);
}

std::optional<domain::Alert> AlertService::resolve(const std::string& id) {
    auto alert = repository_->findById(id);
    if (!alert) {
        return std::nullopt;
    }
    alert->state = domain::AlertState::Resolved;
    return repository_->save(*alert);
}

std::optional<domain::Alert> AlertService::close(const std::string& id) {
    auto alert = repository_->findById(id);
    if (!alert) {
        return std::nullopt;
    }
    alert->state = domain::AlertState::Closed;
    return repository_->save(*alert);
}

}  // namespace induspilot::modules
