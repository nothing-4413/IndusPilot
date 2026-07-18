#pragma once

#include "induspilot/modules/service_status.hpp"

#include <string>
#include <vector>

namespace induspilot::modules {

class IdentityService {
public:
    ServiceStatus status() const;
    bool authenticate(const std::string& username, const std::string& password) const;
    bool hasPermission(const std::vector<std::string>& permissions, const std::string& required) const;
};

}  // namespace induspilot::modules