#include "induspilot/modules/maintenance_service.hpp"

namespace induspilot::modules {

ServiceStatus MaintenanceService::status() const {
    return ServiceStatus{"maintenance-workflow", true, "维护工单模块占位就绪"};
}

domain::WorkOrder MaintenanceService::createFromAlert(const domain::Alert& alert, const std::string& summary) const {
    return domain::WorkOrder{"wo-from-" + alert.id, alert.assetId, alert.id, domain::WorkOrderState::Created, "", summary};
}

}  // namespace induspilot::modules