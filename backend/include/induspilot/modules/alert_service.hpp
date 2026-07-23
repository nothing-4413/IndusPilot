#pragma once

#include "induspilot/data/repositories.hpp"
#include "induspilot/domain/domain_types.hpp"
#include "induspilot/modules/service_status.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace induspilot::modules {

struct AlertQuery {
    std::optional<std::string> assetId;
    std::optional<domain::AlertSeverity> severity;
    std::optional<domain::AlertState> state;
};

struct NotificationDispatchSummary {
    int sent{0};
    int failed{0};
    int skipped{0};
};
class AlertService {
public:
    AlertService();
    explicit AlertService(std::shared_ptr<data::AlertRepository> repository);

    ServiceStatus status() const;
    domain::Alert create(domain::Alert alert);
    std::optional<domain::Alert> findById(const std::string& id) const;
    std::vector<domain::Alert> list(const AlertQuery& query = {}) const;
    domain::AlertRule createRule(domain::AlertRule rule);
    std::vector<domain::AlertRule> rules() const;
    std::vector<domain::AlertNotification> notifications() const;
    NotificationDispatchSummary dispatchQueuedNotifications();
    std::optional<domain::AlertNotification> retryNotification(const std::string& id);
    std::optional<domain::Alert> acknowledge(const std::string& id, const std::string& operatorId);
    std::optional<domain::Alert> assign(const std::string& id, const std::string& assignee);
    std::optional<domain::Alert> resolve(const std::string& id);
    std::optional<domain::Alert> close(const std::string& id);

private:
    void createNotificationsFor(const domain::Alert& alert);
    domain::AlertNotification deliverNotification(domain::AlertNotification notification);

    std::shared_ptr<data::AlertRepository> repository_;
};

std::optional<domain::AlertSeverity> alertSeverityFromString(const std::string& value);
std::optional<domain::AlertState> alertStateFromString(const std::string& value);
std::string alertSeverityToString(domain::AlertSeverity severity);
std::string alertStateToString(domain::AlertState state);

}  // namespace induspilot::modules