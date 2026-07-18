#include "induspilot/modules/alert_service.hpp"

namespace induspilot::modules {
namespace {

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

ServiceStatus AlertService::status() const {
    return ServiceStatus{"alert-management", true, "alert lifecycle service is ready"};
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

std::vector<domain::Alert> AlertService::list(const AlertQuery& query) const {
    std::vector<domain::Alert> result;
    for (const auto& item : alerts_) {
        if (matches(item.second, query)) {
            result.push_back(item.second);
        }
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