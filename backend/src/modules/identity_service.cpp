#include "induspilot/modules/identity_service.hpp"

#include "induspilot/data/in_memory_repositories.hpp"
#include "induspilot/modules/password_hasher.hpp"

#include <algorithm>
#include <chrono>
#include <sstream>
#include <utility>

namespace induspilot::modules {

IdentityService::IdentityService()
    : IdentityService(std::make_shared<InMemorySessionStore>(), std::chrono::hours(8)) {}

IdentityService::IdentityService(std::shared_ptr<SessionStore> sessionStore, std::chrono::seconds sessionTtl)
    : IdentityService(
          std::move(sessionStore),
          sessionTtl,
          std::make_shared<data::InMemoryUserRepository>(),
          std::make_shared<data::InMemoryPermissionRepository>()) {}

IdentityService::IdentityService(
    std::shared_ptr<SessionStore> sessionStore,
    std::chrono::seconds sessionTtl,
    std::shared_ptr<data::UserRepository> userRepository,
    std::shared_ptr<data::PermissionRepository> permissionRepository)
    : sessionStore_(std::move(sessionStore)),
      sessionTtl_(sessionTtl),
      userRepository_(std::move(userRepository)),
      permissionRepository_(std::move(permissionRepository)) {
    if (!sessionStore_) {
        sessionStore_ = std::make_shared<InMemorySessionStore>();
    }
    if (!userRepository_) {
        userRepository_ = std::make_shared<data::InMemoryUserRepository>();
    }
    if (!permissionRepository_) {
        permissionRepository_ = std::make_shared<data::InMemoryPermissionRepository>();
    }
}

ServiceStatus IdentityService::status() const {
    return ServiceStatus{"identity-access", true, "identity repositories and session storage are ready"};
}

AuthResult IdentityService::login(const LoginRequest& request) {
    const auto credential = userRepository_->findByUsername(request.username);
    if (!credential || !verifyPassword(request.password, credential->passwordHash)) {
        return AuthResult{false, "用户名或密码错误", std::nullopt};
    }

    const auto token = issueToken(request.username);
    auto session = SessionInfo{token, credential->user, true};
    if (!sessionStore_->save(session, sessionTtl_)) {
        return AuthResult{false, "会话创建失败", std::nullopt};
    }

    return AuthResult{true, "登录成功", session};
}

bool IdentityService::logout(const std::string& token) {
    if (token.empty()) {
        return false;
    }
    return sessionStore_->remove(token);
}

std::optional<SessionInfo> IdentityService::validateSession(const std::string& token) const {
    if (token.empty()) {
        return std::nullopt;
    }
    return sessionStore_->find(token);
}

bool IdentityService::authenticate(const std::string& username, const std::string& password) const {
    const auto credential = userRepository_->findByUsername(username);
    return credential.has_value() && verifyPassword(password, credential->passwordHash);
}

bool IdentityService::hasPermission(const std::vector<std::string>& permissions, const std::string& required) const {
    return std::find(permissions.begin(), permissions.end(), required) != permissions.end();
}

std::vector<std::string> IdentityService::permissionsForRoles(const std::vector<std::string>& roles) const {
    auto permissions = permissionRepository_->permissionsForRoles(roles);
    std::sort(permissions.begin(), permissions.end());
    permissions.erase(std::unique(permissions.begin(), permissions.end()), permissions.end());
    return permissions;
}

std::string IdentityService::issueToken(const std::string& username) {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

    std::ostringstream token;
    token << "session-" << username << '-' << millis << '-' << ++issuedSessions_;
    return token.str();
}

}  // namespace induspilot::modules