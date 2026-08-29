#pragma once

#include "induspilot/app/config.hpp"
#include "induspilot/data/repositories.hpp"
#include "induspilot/domain/domain_types.hpp"
#include "induspilot/modules/metrics_service.hpp"
#include "induspilot/modules/service_status.hpp"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <cstddef>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace induspilot::modules {

struct OperationAuditIntegrityReport {
    bool verified{true};
    std::size_t total{0};
    std::string brokenEventId;
    std::string latestHash;
};

struct OperationAuditQuery {
    std::optional<std::string> actor;
    std::optional<std::string> action;
    std::optional<std::string> resourceType;
    std::optional<std::string> result;
    std::optional<std::string> occurredFrom;
    std::optional<std::string> occurredTo;
};

struct AuditDeliveryResult {
    bool delivered{false};
    std::string error;
};

class AuditDeliverySink {
public:
    virtual ~AuditDeliverySink() = default;
    virtual AuditDeliveryResult deliver(const domain::OperationAuditEvent& event) const = 0;
};

std::shared_ptr<AuditDeliverySink> makeAuditDeliverySink(const app::AuditConfig& config);

class AuditService {
public:
    AuditService();
    explicit AuditService(std::shared_ptr<data::OperationAuditRepository> repository,
        std::shared_ptr<AuditDeliverySink> deliverySink = nullptr,
        app::AuditConfig config = {},
        std::shared_ptr<data::AuditDeliveryQueueRepository> deliveryQueue = nullptr,
        std::shared_ptr<MetricsRegistry> metrics = nullptr);
    ~AuditService();

    ServiceStatus status() const;
    domain::OperationAuditEvent record(domain::OperationAuditEvent event);
    std::vector<domain::OperationAuditEvent> events(const OperationAuditQuery& query = {}) const;
    std::vector<domain::OperationAuditEvent> archiveEvents(const OperationAuditQuery& query = {}) const;
    OperationAuditIntegrityReport integrityReport() const;

private:
    void deliveryLoop();
    void processDeliveryQueue();
    void recordDeliveryQueueDepths() const;

    mutable std::mutex mutex_;
    std::shared_ptr<data::OperationAuditRepository> repository_;
    std::shared_ptr<AuditDeliverySink> deliverySink_;
    app::AuditConfig config_;
    std::shared_ptr<data::AuditDeliveryQueueRepository> deliveryQueue_;
    std::shared_ptr<MetricsRegistry> metrics_;
    std::thread deliveryWorker_;
    mutable std::mutex deliveryMutex_;
    std::condition_variable deliveryWakeup_;
    bool stopDeliveryWorker_{false};
};

}  // namespace induspilot::modules
