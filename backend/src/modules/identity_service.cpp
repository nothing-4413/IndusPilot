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
    LoginSecurityPolicy securityPolicy,
    PasswordPolicy passwordPolicy,
    std::shared_ptr<LoginRateLimiter> loginRateLimiter)
    : sessionStore_(std::move(sessionStore)),
      sessionTtl_(sessionTtl),
      userRepository_(std::move(userRepository)),
      permissionRepository_(std::move(permissionRepository)),
      loginRateLimiter_(std::move(loginRateLimiter)),
      securityPolicy_(securityPolicy),
      passwordPolicy_(passwordPolicy) {
    if (!sessionStore_) {
        sessionStore_ = std::make_shared<InMemorySessionStore>();
    }
    if (!userRepository_) {
        userRepository_ = std::make_shared<data::InMemoryUserRepository>();
    }
    if (!permissionRepository_) {
        permissionRepository_ = std::make_shared<data::InMemoryPermissionRepository>();
    }
    if (!loginRateLimiter_) {
        loginRateLimiter_ = std::make_shared<InMemoryLoginRateLimiter>();
    }
}

ServiceStatus IdentityService::status() const {
    return ServiceStatus{"identity-access", true, "identity repositories and session storage are ready"};
}

AuthResult IdentityService::login(const LoginRequest& request) {
    const auto now = std::chrono::system_clock::now();
    if (securityPolicy_.enabled) {
        const auto limit = loginRateLimiter_->check(request.username, now);
        if (!limit.available) {
            return AuthResult{false, "登录安全策略暂不可用", std::nullopt, "AUTHENTICATION_RATE_LIMITER_UNAVAILABLE"};
        }
        if (limit.retryAfterSeconds > 0) {
            return AuthResult{false, "登录失败次数过多，请稍后重试", std::nullopt, "AUTHENTICATION_LOCKED", limit.retryAfterSeconds};
        }
    }

    const auto credential = userRepository_->findByUsername(request.username);
    if (!credential || !verifyPassword(request.password, credential->passwordHash)) {
        if (!securityPolicy_.enabled) {
            return AuthResult{false, "用户名或密码错误", std::nullopt, "AUTHENTICATION_FAILED"};
        }
        const auto limit = loginRateLimiter_->recordFailure(
            request.username,
            now,
            securityPolicy_.maxFailures,
            securityPolicy_.failureWindow,
            securityPolicy_.lockDuration);
        if (!limit.available) {
            return AuthResult{false, "登录安全策略暂不可用", std::nullopt, "AUTHENTICATION_RATE_LIMITER_UNAVAILABLE"};
        }
        if (limit.retryAfterSeconds > 0) {
            return AuthResult{false, "登录失败次数过多，请稍后重试", std::nullopt, "AUTHENTICATION_LOCKED", limit.retryAfterSeconds};
        }
        return AuthResult{false, "用户名或密码错误", std::nullopt, "AUTHENTICATION_FAILED"};
    }

    if (credential->requiresPasswordRotation && !securityPolicy_.allowSeedCredentials) {
        return AuthResult{false, "该账号必须先完成密码轮换", std::nullopt, "SEED_CREDENTIAL_ROTATION_REQUIRED"};
    }

    if (securityPolicy_.enabled && !loginRateLimiter_->clear(request.username)) {
        return AuthResult{false, "登录安全策略暂不可用", std::nullopt, "AUTHENTICATION_RATE_LIMITER_UNAVAILABLE"};
    }

    const auto token = issueToken();
    auto session = SessionInfo{token, credential->user, true, credential->credentialVersion};
    if (!sessionStore_->save(session, sessionTtl_)) {
        return AuthResult{false, "会话创建失败", std::nullopt, "SESSION_CREATE_FAILED"};
    }

    return AuthResult{true, "登录成功", session, "OK"};
}

PasswordChangeResult IdentityService::changePassword(
    const std::string& username,
    const std::string& currentPassword,
    const std::string& newPassword) {
    const auto credential = userRepository_->findByUsername(username);
    if (!credential || !verifyPassword(currentPassword, credential->passwordHash)) {
        return PasswordChangeResult{false, "当前密码不正确", "CURRENT_PASSWORD_INVALID"};
    }
    if (static_cast<int>(newPassword.size()) < passwordPolicy_.minimumLength) {
        return PasswordChangeResult{false, "新密码不符合密码策略", "PASSWORD_POLICY_VIOLATION"};
    }

    const auto passwordHash = generatePbkdf2Sha256PasswordHash(newPassword, passwordPolicy_.iterations);
    if (passwordHash.empty()) {
        return PasswordChangeResult{false, "密码哈希生成失败", "PASSWORD_HASH_FAILED"};
    }
    if (!userRepository_->updatePasswordHash(username, passwordHash)) {
        return PasswordChangeResult{false, "密码更新失败", "PASSWORD_UPDATE_FAILED"};
    }
    if (!sessionStore_->removeForUser(credential->user.id)) {
        return PasswordChangeResult{false, "密码已更新，但会话撤销失败", "SESSION_REVOCATION_FAILED"};
    }
    return PasswordChangeResult{true, "密码修改成功", "OK"};
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
    const auto session = sessionStore_->find(token);
    if (!session) {
        return std::nullopt;
    }
    const auto credential = userRepository_->findByUsername(session->user.username);
    if (!credential || credential->credentialVersion != session->credentialVersion) {
        return std::nullopt;
    }
    return session;
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
