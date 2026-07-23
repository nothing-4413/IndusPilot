#include "induspilot/modules/maintenance_service.hpp"

#include "induspilot/data/in_memory_repositories.hpp"

#include <utility>

namespace induspilot::modules {
namespace {

bool matches(const domain::WorkOrder& order, const WorkOrderQuery& query) {
    if (query.assetId && order.assetId != *query.assetId) {
        return false;
    }
    if (query.alertId && order.alertId != *query.alertId) {
        return false;
    }
    if (query.state && order.state != *query.state) {
        return false;
    }
    return true;
}

}  // namespace

std::optional<domain::WorkOrderState> workOrderStateFromString(const std::string& value) {
    if (value == "created") {
        return domain::WorkOrderState::Created;
    }
    if (value == "assigned") {
        return domain::WorkOrderState::Assigned;
    }
    if (value == "processing") {
        return domain::WorkOrderState::Processing;
    }
    if (value == "completed") {
        return domain::WorkOrderState::Completed;
    }
    if (value == "closed") {
        return domain::WorkOrderState::Closed;
    }
    return std::nullopt;
}

std::string workOrderStateToString(domain::WorkOrderState state) {
    switch (state) {
        case domain::WorkOrderState::Created:
            return "created";
        case domain::WorkOrderState::Assigned:
            return "assigned";
        case domain::WorkOrderState::Processing:
            return "processing";
        case domain::WorkOrderState::Completed:
            return "completed";
        case domain::WorkOrderState::Closed:
            return "closed";
    }
    return "unknown";
}

MaintenanceService::MaintenanceService() : MaintenanceService(std::make_shared<data::InMemoryWorkOrderRepository>()) {}

MaintenanceService::MaintenanceService(std::shared_ptr<data::WorkOrderRepository> repository) : repository_(std::move(repository)) {
    if (!repository_) {
        repository_ = std::make_shared<data::InMemoryWorkOrderRepository>();
    }
}

ServiceStatus MaintenanceService::status() const {
    return ServiceStatus{"maintenance-workflow", true, "work order repository is ready"};
}

domain::WorkOrder MaintenanceService::create(domain::WorkOrder order) {
    return repository_->save(std::move(order));
}

domain::WorkOrder MaintenanceService::createFromAlert(const domain::Alert& alert, const std::string& summary) {
    return create(domain::WorkOrder{"wo-from-" + alert.id, alert.assetId, alert.id, domain::WorkOrderState::Created, "", summary, ""});
}

std::optional<domain::WorkOrder> MaintenanceService::findById(const std::string& id) const {
    return repository_->findById(id);
}

std::optional<domain::WorkOrder> MaintenanceService::assign(const std::string& id, const std::string& assignee) {
    auto order = repository_->findById(id);
    if (!order) {
        return std::nullopt;
    }
    order->state = domain::WorkOrderState::Assigned;
    order->assignee = assignee;
    return repository_->save(*order);
}

std::optional<domain::WorkOrder> MaintenanceService::startProcessing(const std::string& id) {
    auto order = repository_->findById(id);
    if (!order) {
        return std::nullopt;
    }
    order->state = domain::WorkOrderState::Processing;
    return repository_->save(*order);
}

std::optional<domain::WorkOrder> MaintenanceService::complete(const std::string& id, const std::string& result) {
    auto order = repository_->findById(id);
    if (!order) {
        return std::nullopt;
    }
    order->state = domain::WorkOrderState::Completed;
    order->result = result;
    return repository_->save(*order);
}

std::optional<domain::WorkOrder> MaintenanceService::close(const std::string& id) {
    auto order = repository_->findById(id);
    if (!order) {
        return std::nullopt;
    }
    order->state = domain::WorkOrderState::Closed;
    return repository_->save(*order);
}

std::vector<domain::WorkOrder> MaintenanceService::list(const WorkOrderQuery& query) const {
    std::vector<domain::WorkOrder> result;
    for (const auto& order : repository_->list()) {
        if (matches(order, query)) {
            result.push_back(order);
        }
    }
    return result;
}

std::vector<domain::WorkOrder> MaintenanceService::historyForAsset(const std::string& assetId) const {
    return repository_->historyForAsset(assetId);
}

}  // namespace induspilot::modules