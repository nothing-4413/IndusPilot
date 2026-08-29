#pragma once

#include "induspilot/app/config.hpp"
#include "induspilot/data/repositories.hpp"
#include "induspilot/domain/domain_types.hpp"
#include "induspilot/modules/service_status.hpp"

#include <memory>
#include <mutex>
#include <cstddef>
#include <optional>
#include <string>
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

class AuditDeliverySink {
public:
    virtual ~AuditDeliverySink() = default;
    virtual void deliver(const domain::OperationAuditEvent& event) const = 0;
};

std::shared_ptr<AuditDeliverySink> makeAuditDeliverySink(const app::AuditConfig& config);

class AuditService {
public:
    AuditService();
    explicit AuditService(std::shared_ptr<data::OperationAuditRepository> repository,
        std::shared_ptr<AuditDeliverySink> deliverySink = nullptr,
        app::AuditConfig config = {});

    ServiceStatus status() const;
    domain::OperationAuditEvent record(domain::OperationAuditEvent event);
    std::vector<domain::OperationAuditEvent> events(const OperationAuditQuery& query = {}) const;
    std::vector<domain::OperationAuditEvent> archiveEvents(const OperationAuditQuery& query = {}) const;
    OperationAuditIntegrityReport integrityReport() const;

private:
    mutable std::mutex mutex_;
    std::shared_ptr<data::OperationAuditRepository> repository_;
    std::shared_ptr<AuditDeliverySink> deliverySink_;
    app::AuditConfig config_;
};

}  // namespace induspilot::modules
