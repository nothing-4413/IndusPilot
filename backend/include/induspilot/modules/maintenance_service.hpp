#pragma once

#include "induspilot/data/repositories.hpp"
#include "induspilot/domain/domain_types.hpp"
#include "induspilot/modules/service_status.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace induspilot::modules {

struct WorkOrderQuery {
    std::optional<std::string> assetId;
    std::optional<std::string> alertId;
    std::optional<domain::WorkOrderState> state;
};

class MaintenanceService {
public:
    MaintenanceService();
    explicit MaintenanceService(std::shared_ptr<data::WorkOrderRepository> repository);

    ServiceStatus status() const;
    domain::WorkOrder create(domain::WorkOrder order);
    domain::WorkOrder createFromAlert(const domain::Alert& alert, const std::string& summary);
    std::optional<domain::WorkOrder> findById(const std::string& id) const;
    std::optional<domain::WorkOrder> assign(const std::string& id, const std::string& assignee);
    std::optional<domain::WorkOrder> startProcessing(const std::string& id);
    std::optional<domain::WorkOrder> complete(const std::string& id, const std::string& result);
    std::optional<domain::WorkOrder> close(const std::string& id);
    std::vector<domain::WorkOrder> list(const WorkOrderQuery& query = {}) const;
    std::vector<domain::WorkOrder> historyForAsset(const std::string& assetId) const;

private:
    std::shared_ptr<data::WorkOrderRepository> repository_;
};

std::optional<domain::WorkOrderState> workOrderStateFromString(const std::string& value);
std::string workOrderStateToString(domain::WorkOrderState state);

}  // namespace induspilot::modules