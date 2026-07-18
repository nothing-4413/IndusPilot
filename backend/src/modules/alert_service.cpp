#include "induspilot/modules/alert_service.hpp"

namespace induspilot::modules {

ServiceStatus AlertService::status() const {
    return ServiceStatus{"alert-management", true, "告警管理模块占位就绪"};
}

domain::Alert AlertService::acknowledge(domain::Alert alert, const std::string& operatorId) const {
    alert.state = domain::AlertState::Acknowledged;
    alert.acknowledgedBy = operatorId;
    return alert;
}

}  // namespace induspilot::modules