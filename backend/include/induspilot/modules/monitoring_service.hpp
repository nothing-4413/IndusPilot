#pragma once

#include "induspilot/modules/service_status.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace induspilot::modules {

struct RuntimeState {
    std::string assetId;
    std::string state{"unknown"};
    std::string metricSummary;
    std::string updatedAt;
    std::string severity{"info"};
};

class MonitoringService {
public:
    ServiceStatus status() const;
    RuntimeState updateState(RuntimeState state);
    std::optional<RuntimeState> findState(const std::string& assetId) const;
    std::vector<RuntimeState> listStates() const;
    std::map<std::string, int> summarizeStates() const;
    std::map<std::string, int> summarizeSeverity() const;

private:
    std::map<std::string, RuntimeState> states_;
};

bool isSupportedRuntimeState(const std::string& state);
bool isSupportedRuntimeSeverity(const std::string& severity);

}  // namespace induspilot::modules