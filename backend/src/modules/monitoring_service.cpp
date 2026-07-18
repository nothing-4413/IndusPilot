#include "induspilot/modules/monitoring_service.hpp"

namespace induspilot::modules {

ServiceStatus MonitoringService::status() const {
    return ServiceStatus{"operational-monitoring", true, "运行监控模块占位就绪"};
}

RuntimeState MonitoringService::updateState(RuntimeState state) {
    states_[state.assetId] = state;
    return state;
}

std::optional<RuntimeState> MonitoringService::findState(const std::string& assetId) const {
    const auto it = states_.find(assetId);
    if (it == states_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::map<std::string, int> MonitoringService::summarizeStates() const {
    std::map<std::string, int> summary{{"online", 0}, {"warning", 0}, {"critical", 0}, {"offline", 0}};
    for (const auto& item : states_) {
        summary[item.second.state]++;
    }
    return summary;
}

}  // namespace induspilot::modules