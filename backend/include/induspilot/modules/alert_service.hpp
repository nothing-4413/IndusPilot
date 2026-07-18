#pragma once

#include "induspilot/domain/domain_types.hpp"
#include "induspilot/modules/service_status.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace induspilot::modules {

class AlertService {
public:
    ServiceStatus status() const;
    domain::Alert create(domain::Alert alert);
    std::optional<domain::Alert> findById(const std::string& id) const;
    std::vector<domain::Alert> list() const;
    std::optional<domain::Alert> acknowledge(const std::string& id, const std::string& operatorId);
    std::optional<domain::Alert> assign(const std::string& id, const std::string& assignee);
    std::optional<domain::Alert> resolve(const std::string& id);
    std::optional<domain::Alert> close(const std::string& id);

private:
    std::map<std::string, domain::Alert> alerts_;
};

}  // namespace induspilot::modules