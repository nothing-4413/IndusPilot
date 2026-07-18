#include "induspilot/modules/maintenance_service.hpp"

namespace induspilot::modules {

ServiceStatus MaintenanceService::status() const {
    return ServiceStatus{"maintenance-workflow", true, "缁存姢宸ュ崟妯″潡鍗犱綅灏辩华"};
}

domain::WorkOrder MaintenanceService::create(domain::WorkOrder order) {
    orders_[order.id] = order;
    return order;
}

domain::WorkOrder MaintenanceService::createFromAlert(const domain::Alert& alert, const std::string& summary) {
    return create(domain::WorkOrder{"wo-from-" + alert.id, alert.assetId, alert.id, domain::WorkOrderState::Created, "", summary, ""});
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

std::vector<domain::WorkOrder> MaintenanceService::list() const {
    std::vector<domain::WorkOrder> result;
    for (const auto& item : orders_) {
        result.push_back(item.second);
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