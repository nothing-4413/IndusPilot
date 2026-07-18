#include "induspilot/modules/monitoring_service.hpp"

namespace induspilot::modules {

ServiceStatus MonitoringService::status() const {
    return ServiceStatus{"operational-monitoring", true, "运行监控模块占位就绪"};
}

std::map<std::string, int> MonitoringService::summarizeStates() const {
    return {{"online", 0}, {"warning", 0}, {"critical", 0}, {"offline", 0}};
}

}  // namespace induspilot::modules