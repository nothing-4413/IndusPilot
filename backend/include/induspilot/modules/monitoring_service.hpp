#pragma once

#include "induspilot/modules/service_status.hpp"

#include <map>
#include <optional>
#include <string>

namespace induspilot::modules {

struct RuntimeState {
    std::string assetId;
    std::string state{"unknown"};
    std::string metricSummary;
    std::string updatedAt;
};

class MonitoringService {
public:
    ServiceStatus status() const;
    RuntimeState updateState(RuntimeState state);
    std::optional<RuntimeState> findState(const std::string& assetId) const;
    std::map<std::string, int> summarizeStates() const;

private:
    std::map<std::string, RuntimeState> states_;
};

}  // namespace induspilot::modules