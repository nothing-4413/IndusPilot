#include "induspilot/modules/alert_service.hpp"

#include "induspilot/data/in_memory_repositories.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
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

bool isSupportedNotificationChannel(const std::string& channel) {
    return channel == "console" || channel == "email" || channel == "webhook";
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

AlertService::AlertService(std::shared_ptr<data::AlertRepository> repository) : repository_(std::move(repository)) {
    if (!repository_) {
        repository_ = std::make_shared<data::InMemoryAlertRepository>();
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
    for (auto notification : repository_->listNotifications()) {
        if (notification.status != "queued" && notification.status != "retrying") {
            ++summary.skipped;
            continue;
        }
        notification = deliverNotification(std::move(notification));
        if (notification.status == "sent") {
            ++summary.sent;
        } else if (notification.status == "failed") {
            ++summary.failed;
        } else {
            ++summary.skipped;
        }
    }
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
    if (notification.channel.empty() || notification.target.empty()) {
        notification.status = "failed";
        notification.lastError = "通知通道和目标不能为空";
        notification.deliveredAt.clear();
        return repository_->saveNotification(std::move(notification));
    }
    if (!isSupportedNotificationChannel(notification.channel)) {
        notification.status = "failed";
        notification.lastError = "不支持的通知通道：" + notification.channel;
        notification.deliveredAt.clear();
        return repository_->saveNotification(std::move(notification));
    }

    notification.status = "sent";
    notification.lastError.clear();
    notification.deliveredAt = currentTimestamp();
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
