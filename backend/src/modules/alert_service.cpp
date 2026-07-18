#include "induspilot/modules/alert_service.hpp"

namespace induspilot::modules {

ServiceStatus AlertService::status() const {
    return ServiceStatus{"alert-management", true, "告警管理模块占位就绪"};
}

domain::Alert AlertService::create(domain::Alert alert) {
    alerts_[alert.id] = alert;
    return alert;
}

std::optional<domain::Alert> AlertService::findById(const std::string& id) const {
    const auto it = alerts_.find(id);
    if (it == alerts_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<domain::Alert> AlertService::list() const {
    std::vector<domain::Alert> result;
    for (const auto& item : alerts_) {
        result.push_back(item.second);
    }
    return result;
}

std::optional<domain::Alert> AlertService::acknowledge(const std::string& id, const std::string& operatorId) {
    const auto it = alerts_.find(id);
    if (it == alerts_.end()) {
        return std::nullopt;
    }
    it->second.state = domain::AlertState::Acknowledged;
    it->second.acknowledgedBy = operatorId;
    return it->second;
}

std::optional<domain::Alert> AlertService::assign(const std::string& id, const std::string& assignee) {
    const auto it = alerts_.find(id);
    if (it == alerts_.end()) {
        return std::nullopt;
    }
    it->second.state = domain::AlertState::Assigned;
    it->second.assignedTo = assignee;
    return it->second;
}

std::optional<domain::Alert> AlertService::resolve(const std::string& id) {
    const auto it = alerts_.find(id);
    if (it == alerts_.end()) {
        return std::nullopt;
    }
    it->second.state = domain::AlertState::Resolved;
    return it->second;
}

std::optional<domain::Alert> AlertService::close(const std::string& id) {
    const auto it = alerts_.find(id);
    if (it == alerts_.end()) {
        return std::nullopt;
    }
    it->second.state = domain::AlertState::Closed;
    return it->second;
}

}  // namespace induspilot::modules