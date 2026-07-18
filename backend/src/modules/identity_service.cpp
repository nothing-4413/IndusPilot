#include "induspilot/modules/identity_service.hpp"

#include <algorithm>

namespace induspilot::modules {

ServiceStatus IdentityService::status() const {
    return ServiceStatus{"identity-access", true, "身份认证与权限模块占位就绪"};
}

bool IdentityService::authenticate(const std::string& username, const std::string& password) const {
    return !username.empty() && !password.empty();
}

bool IdentityService::hasPermission(const std::vector<std::string>& permissions, const std::string& required) const {
    return std::find(permissions.begin(), permissions.end(), required) != permissions.end();
}

}  // namespace induspilot::modules