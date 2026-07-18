#pragma once

#include "induspilot/domain/domain_types.hpp"
#include "induspilot/modules/service_status.hpp"

namespace induspilot::modules {

class MaintenanceService {
public:
    ServiceStatus status() const;
    domain::WorkOrder createFromAlert(const domain::Alert& alert, const std::string& summary) const;
};

}  // namespace induspilot::modules