#pragma once

#include "induspilot/data/repositories.hpp"
#include "induspilot/domain/domain_types.hpp"
#include "induspilot/modules/service_status.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace induspilot::modules {

struct OperationAuditQuery {
    std::optional<std::string> actor;
    std::optional<std::string> action;
    std::optional<std::string> resourceType;
    std::optional<std::string> result;
};

class AuditService {
public:
    AuditService();
    explicit AuditService(std::shared_ptr<data::OperationAuditRepository> repository);

    ServiceStatus status() const;
    domain::OperationAuditEvent record(domain::OperationAuditEvent event);
    std::vector<domain::OperationAuditEvent> events(const OperationAuditQuery& query = {}) const;

private:
    std::shared_ptr<data::OperationAuditRepository> repository_;
};

}  // namespace induspilot::modules