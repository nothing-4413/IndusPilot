#pragma once

#include "induspilot/domain/domain_types.hpp"
#include "induspilot/modules/service_status.hpp"

namespace induspilot::modules {

class AlertService {
public:
    ServiceStatus status() const;
    domain::Alert acknowledge(domain::Alert alert, const std::string& operatorId) const;
};

}  // namespace induspilot::modules