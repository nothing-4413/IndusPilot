#pragma once

#include "induspilot/modules/service_status.hpp"

#include <map>
#include <string>

namespace induspilot::modules {

class MonitoringService {
public:
    ServiceStatus status() const;
    std::map<std::string, int> summarizeStates() const;
};

}  // namespace induspilot::modules