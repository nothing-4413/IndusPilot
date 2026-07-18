#include "induspilot/modules/identity_service.hpp"

#include <algorithm>
#include <sstream>

namespace induspilot::modules {

IdentityService::IdentityService() {
    users_["admin"] = domain::User{"user-admin", "admin", {"admin"}};
    users_["operator"] = domain::User{"user-operator", "operator", {"operator"}};
    users_["maintainer"] = domain::User{"user-maintainer", "maintainer", {"maintainer"}};

    // 当前为开发骨架口令，生产实现必须替换为加盐哈希和密码策略。
    passwordHashes_["admin"] = "admin123";
    passwordHashes_["operator"] = "operator123";
    passwordHashes_["maintainer"] = "maintainer123";

    rolePermissions_["admin"] = {"asset:read", "asset:write", "alert:read", "alert:write", "work-order:read", "work-order:write", "ai:use"};
    rolePermissions_["operator"] = {"asset:read", "alert:read", "alert:write", "work-order:read", "ai:use"};
    rolePermissions_["maintainer"] = {"asset:read", "alert:read", "work-order:read", "work-order:write", "ai:use"};
}

ServiceStatus IdentityService::status() const {
    return ServiceStatus{"identity-access", true, "身份认证、会话和权限模块占位就绪"};
}

AuthResult IdentityService::login(const LoginRequest& request) {
    if (!authenticate(request.username, request.password)) {
        return AuthResult{false, "用户名或密码错误", std::nullopt};
    }

    const auto token = issueToken(request.username);
    auto session = SessionInfo{token, users_.at(request.username), true};
    sessions_[token] = session;
    return AuthResult{true, "登录成功", session};
}

bool IdentityService::logout(const std::string& token) {
    const auto it = sessions_.find(token);
    if (it == sessions_.end()) {
        return false;
    }
    it->second.active = false;
    return true;
}

std::optional<SessionInfo> IdentityService::validateSession(const std::string& token) const {
    const auto it = sessions_.find(token);
    if (it == sessions_.end() || !it->second.active) {
        return std::nullopt;
    }
    return it->second;
}

bool IdentityService::authenticate(const std::string& username, const std::string& password) const {
    const auto it = passwordHashes_.find(username);
    return it != passwordHashes_.end() && it->second == password;
}

bool IdentityService::hasPermission(const std::vector<std::string>& permissions, const std::string& required) const {
    return std::find(permissions.begin(), permissions.end(), required) != permissions.end();
}

std::vector<std::string> IdentityService::permissionsForRoles(const std::vector<std::string>& roles) const {
    std::vector<std::string> permissions;
    for (const auto& role : roles) {
        const auto it = rolePermissions_.find(role);
        if (it == rolePermissions_.end()) {
            continue;
        }
        permissions.insert(permissions.end(), it->second.begin(), it->second.end());
    }
    std::sort(permissions.begin(), permissions.end());
    permissions.erase(std::unique(permissions.begin(), permissions.end()), permissions.end());
    return permissions;
}

std::string IdentityService::issueToken(const std::string& username) const {
    std::ostringstream token;
    token << "dev-session-" << username << "-" << sessions_.size() + 1;
    return token.str();
}

}  // namespace induspilot::modules