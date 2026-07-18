#pragma once

#include "induspilot/domain/domain_types.hpp"
#include "induspilot/modules/service_status.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace induspilot::modules {

class MaintenanceService {
public:
    ServiceStatus status() const;
    domain::WorkOrder create(domain::WorkOrder order);
    domain::WorkOrder createFromAlert(const domain::Alert& alert, const std::string& summary);
    std::optional<domain::WorkOrder> assign(const std::string& id, const std::string& assignee);
    std::optional<domain::WorkOrder> startProcessing(const std::string& id);
    std::optional<domain::WorkOrder> complete(const std::string& id, const std::string& result);
    std::optional<domain::WorkOrder> close(const std::string& id);
    std::vector<domain::WorkOrder> historyForAsset(const std::string& assetId) const;

private:
    std::map<std::string, domain::WorkOrder> orders_;
};

}  // namespace induspilot::modules