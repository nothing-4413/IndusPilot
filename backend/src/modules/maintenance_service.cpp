#include "induspilot/modules/maintenance_service.hpp"

namespace induspilot::modules {
namespace {

bool matches(const domain::WorkOrder& order, const WorkOrderQuery& query) {
    if (query.assetId && order.assetId != *query.assetId) {
        return false;
    }
    if (query.alertId && order.alertId != *query.alertId) {
        return false;
    }
    if (query.state && order.state != *query.state) {
        return false;
    }
    return true;
}

}  // namespace

std::optional<domain::WorkOrderState> workOrderStateFromString(const std::string& value) {
    if (value == "created") {
        return domain::WorkOrderState::Created;
    }
    if (value == "assigned") {
        return domain::WorkOrderState::Assigned;
    }
    if (value == "processing") {
        return domain::WorkOrderState::Processing;
    }
    if (value == "completed") {
        return domain::WorkOrderState::Completed;
    }
    if (value == "closed") {
        return domain::WorkOrderState::Closed;
    }
    return std::nullopt;
}

std::string workOrderStateToString(domain::WorkOrderState state) {
    switch (state) {
        case domain::WorkOrderState::Created:
            return "created";
        case domain::WorkOrderState::Assigned:
            return "assigned";
        case domain::WorkOrderState::Processing:
            return "processing";
        case domain::WorkOrderState::Completed:
            return "completed";
        case domain::WorkOrderState::Closed:
            return "closed";
    }
    return "unknown";
}

ServiceStatus MaintenanceService::status() const {
    return ServiceStatus{"maintenance-workflow", true, "work order lifecycle service is ready"};
}

domain::WorkOrder MaintenanceService::create(domain::WorkOrder order) {
    orders_[order.id] = order;
    return order;
}

domain::WorkOrder MaintenanceService::createFromAlert(const domain::Alert& alert, const std::string& summary) {
    return create(domain::WorkOrder{"wo-from-" + alert.id, alert.assetId, alert.id, domain::WorkOrderState::Created, "", summary, ""});
}

std::optional<domain::WorkOrder> MaintenanceService::findById(const std::string& id) const {
    const auto it = orders_.find(id);
    if (it == orders_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<domain::WorkOrder> MaintenanceService::assign(const std::string& id, const std::string& assignee) {
    const auto it = orders_.find(id);
    if (it == orders_.end()) {
        return std::nullopt;
    }
    it->second.state = domain::WorkOrderState::Assigned;
    it->second.assignee = assignee;
    return it->second;
}

std::optional<domain::WorkOrder> MaintenanceService::startProcessing(const std::string& id) {
    const auto it = orders_.find(id);
    if (it == orders_.end()) {
        return std::nullopt;
    }
    it->second.state = domain::WorkOrderState::Processing;
    return it->second;
}

std::optional<domain::WorkOrder> MaintenanceService::complete(const std::string& id, const std::string& result) {
    const auto it = orders_.find(id);
    if (it == orders_.end()) {
        return std::nullopt;
    }
    it->second.state = domain::WorkOrderState::Completed;
    it->second.result = result;
    return it->second;
}

std::optional<domain::WorkOrder> MaintenanceService::close(const std::string& id) {
    const auto it = orders_.find(id);
    if (it == orders_.end()) {
        return std::nullopt;
    }
    it->second.state = domain::WorkOrderState::Closed;
    return it->second;
}

std::vector<domain::WorkOrder> MaintenanceService::list(const WorkOrderQuery& query) const {
    std::vector<domain::WorkOrder> result;
    for (const auto& item : orders_) {
        if (matches(item.second, query)) {
            result.push_back(item.second);
        }
    }
    return result;
}

std::vector<domain::WorkOrder> MaintenanceService::historyForAsset(const std::string& assetId) const {
    std::vector<domain::WorkOrder> result;
    for (const auto& item : orders_) {
        if (item.second.assetId == assetId && item.second.state == domain::WorkOrderState::Closed) {
            result.push_back(item.second);
        }
    }
    return result;
}

}  // namespace induspilot::modules