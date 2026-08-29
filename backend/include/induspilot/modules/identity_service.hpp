#pragma once

#include "induspilot/data/repositories.hpp"
#include "induspilot/domain/domain_types.hpp"
#include "induspilot/modules/service_status.hpp"
#include "induspilot/modules/login_rate_limiter.hpp"
#include "induspilot/modules/session_store.hpp"

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace induspilot::modules {

struct LoginRequest {
    std::string username;
    std::string password;
};

struct LoginSecurityPolicy {
    bool enabled{true};
    bool allowSeedCredentials{true};
    int maxFailures{5};
    std::chrono::seconds failureWindow{std::chrono::seconds(60)};
    std::chrono::seconds lockDuration{std::chrono::minutes(15)};
};

struct PasswordPolicy {
    int minimumLength{12};
    int iterations{120000};
};

struct AuthResult {
    bool success{false};
    std::string message;
    std::optional<SessionInfo> session;
    std::string code;
    int retryAfterSeconds{0};
};

struct PasswordChangeResult {
    bool success{false};
    std::string message;
    std::string code;
};

class IdentityService {
public:
    IdentityService();
    explicit IdentityService(std::shared_ptr<SessionStore> sessionStore, std::chrono::seconds sessionTtl = std::chrono::hours(8));
    IdentityService(
        std::shared_ptr<SessionStore> sessionStore,
        std::chrono::seconds sessionTtl,
        std::shared_ptr<data::UserRepository> userRepository,
        std::shared_ptr<data::PermissionRepository> permissionRepository,
        LoginSecurityPolicy securityPolicy = {},
        PasswordPolicy passwordPolicy = {},
        std::shared_ptr<LoginRateLimiter> loginRateLimiter = nullptr);

    ServiceStatus status() const;
    AuthResult login(const LoginRequest& request);
    PasswordChangeResult changePassword(
        const std::string& username,
        const std::string& currentPassword,
        const std::string& newPassword);
    bool logout(const std::string& token);
    std::optional<SessionInfo> validateSession(const std::string& token) const;
    bool authenticate(const std::string& username, const std::string& password) const;
    bool hasPermission(const std::vector<std::string>& permissions, const std::string& required) const;
    std::vector<std::string> permissionsForRoles(const std::vector<std::string>& roles) const;

private:
    std::string issueToken();

    std::shared_ptr<SessionStore> sessionStore_;
    std::chrono::seconds sessionTtl_;
    std::shared_ptr<data::UserRepository> userRepository_;
    std::shared_ptr<data::PermissionRepository> permissionRepository_;
    std::shared_ptr<LoginRateLimiter> loginRateLimiter_;
    LoginSecurityPolicy securityPolicy_{};
    PasswordPolicy passwordPolicy_{};
};

}  // namespace induspilot::modules
