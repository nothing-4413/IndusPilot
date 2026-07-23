#include "induspilot/modules/alert_service.hpp"

#include "induspilot/data/in_memory_repositories.hpp"

#include <utility>

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
    return repository_->save(std::move(alert));
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