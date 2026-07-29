#pragma once

#include "induspilot/data/repositories.hpp"
#include "induspilot/domain/domain_types.hpp"
#include "induspilot/modules/service_status.hpp"
#include "induspilot/modules/session_store.hpp"

#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace induspilot::modules {

struct LoginRequest {
    std::string username;
    std::string password;
};

struct LoginSecurityPolicy {
    bool enabled{true};
    int maxFailures{5};
    std::chrono::seconds failureWindow{std::chrono::seconds(60)};
    std::chrono::seconds lockDuration{std::chrono::minutes(15)};
};

struct AuthResult {
    bool success{false};
    std::string message;
    std::optional<SessionInfo> session;
    std::string code;
    int retryAfterSeconds{0};
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
        LoginSecurityPolicy securityPolicy = {});

    ServiceStatus status() const;
    AuthResult login(const LoginRequest& request);
    bool logout(const std::string& token);
    std::optional<SessionInfo> validateSession(const std::string& token) const;
    bool authenticate(const std::string& username, const std::string& password) const;
    bool hasPermission(const std::vector<std::string>& permissions, const std::string& required) const;
    std::vector<std::string> permissionsForRoles(const std::vector<std::string>& roles) const;

private:
    struct LoginFailureState {
        int failures{0};
        std::chrono::system_clock::time_point firstFailureAt{};
        std::chrono::system_clock::time_point lockedUntil{};
    };

    std::string issueToken();
    int retryAfterSeconds(const LoginFailureState& state, std::chrono::system_clock::time_point now) const;
    std::optional<int> lockedRetryAfter(const std::string& username, std::chrono::system_clock::time_point now);
    int recordFailedLogin(const std::string& username, std::chrono::system_clock::time_point now);
    void clearFailedLogin(const std::string& username);

    std::shared_ptr<SessionStore> sessionStore_;
    std::chrono::seconds sessionTtl_;
    std::shared_ptr<data::UserRepository> userRepository_;
    std::shared_ptr<data::PermissionRepository> permissionRepository_;
    LoginSecurityPolicy securityPolicy_{};
    std::mutex loginFailureMutex_;
    std::unordered_map<std::string, LoginFailureState> loginFailures_;
};

}  // namespace induspilot::modules
