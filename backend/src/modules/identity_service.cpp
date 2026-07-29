#include "induspilot/modules/identity_service.hpp"

#include "induspilot/data/in_memory_repositories.hpp"
#include "induspilot/modules/password_hasher.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <iomanip>
#include <random>
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
    std::shared_ptr<data::PermissionRepository> permissionRepository,
    LoginSecurityPolicy securityPolicy)
    : sessionStore_(std::move(sessionStore)),
      sessionTtl_(sessionTtl),
      userRepository_(std::move(userRepository)),
      permissionRepository_(std::move(permissionRepository)),
      securityPolicy_(securityPolicy) {
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
    const auto now = std::chrono::system_clock::now();
    if (const auto retryAfter = lockedRetryAfter(request.username, now)) {
        return AuthResult{false, "登录失败次数过多，请稍后重试", std::nullopt, "AUTHENTICATION_LOCKED", *retryAfter};
    }

    const auto credential = userRepository_->findByUsername(request.username);
    if (!credential || !verifyPassword(request.password, credential->passwordHash)) {
        const auto retryAfter = recordFailedLogin(request.username, now);
        if (retryAfter > 0) {
            return AuthResult{false, "登录失败次数过多，请稍后重试", std::nullopt, "AUTHENTICATION_LOCKED", retryAfter};
        }
        return AuthResult{false, "用户名或密码错误", std::nullopt, "AUTHENTICATION_FAILED"};
    }

    clearFailedLogin(request.username);

    const auto token = issueToken();
    auto session = SessionInfo{token, credential->user, true};
    if (!sessionStore_->save(session, sessionTtl_)) {
        return AuthResult{false, "会话创建失败", std::nullopt, "SESSION_CREATE_FAILED"};
    }

    return AuthResult{true, "登录成功", session, "OK"};
}

int IdentityService::retryAfterSeconds(const LoginFailureState& state, std::chrono::system_clock::time_point now) const {
    if (state.lockedUntil <= now) {
        return 0;
    }
    return static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(state.lockedUntil - now).count()) + 1;
}

std::optional<int> IdentityService::lockedRetryAfter(const std::string& username, std::chrono::system_clock::time_point now) {
    if (!securityPolicy_.enabled || username.empty()) {
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(loginFailureMutex_);
    auto it = loginFailures_.find(username);
    if (it == loginFailures_.end()) {
        return std::nullopt;
    }

    const auto retryAfter = retryAfterSeconds(it->second, now);
    if (retryAfter > 0) {
        return retryAfter;
    }

    if (it->second.lockedUntil.time_since_epoch().count() > 0) {
        loginFailures_.erase(it);
    }
    return std::nullopt;
}

int IdentityService::recordFailedLogin(const std::string& username, std::chrono::system_clock::time_point now) {
    if (!securityPolicy_.enabled || username.empty() || securityPolicy_.maxFailures <= 0) {
        return 0;
    }

    std::lock_guard<std::mutex> lock(loginFailureMutex_);
    auto& state = loginFailures_[username];
    if (state.firstFailureAt.time_since_epoch().count() == 0 || now - state.firstFailureAt > securityPolicy_.failureWindow) {
        state.firstFailureAt = now;
        state.failures = 0;
        state.lockedUntil = {};
    }

    ++state.failures;
    if (state.failures >= securityPolicy_.maxFailures) {
        state.lockedUntil = now + securityPolicy_.lockDuration;
        return retryAfterSeconds(state, now);
    }
    return 0;
}

void IdentityService::clearFailedLogin(const std::string& username) {
    if (!securityPolicy_.enabled || username.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(loginFailureMutex_);
    loginFailures_.erase(username);
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

std::string IdentityService::issueToken() {
    std::array<unsigned char, 32> bytes{};
    std::random_device random;
    for (auto& byte : bytes) {
        byte = static_cast<unsigned char>(random());
    }

    std::ostringstream token;
    token << "session-" << std::hex << std::setfill('0');
    for (const auto byte : bytes) {
        token << std::setw(2) << static_cast<int>(byte);
    }
    return token.str();
}

}  // namespace induspilot::modules