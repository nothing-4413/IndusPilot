#pragma once

#include "induspilot/data/repositories.hpp"
#include "induspilot/domain/domain_types.hpp"
#include "induspilot/modules/service_status.hpp"
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

struct AuthResult {
    bool success{false};
    std::string message;
    std::optional<SessionInfo> session;
};

class IdentityService {
public:
    IdentityService();
    explicit IdentityService(std::shared_ptr<SessionStore> sessionStore, std::chrono::seconds sessionTtl = std::chrono::hours(8));
    IdentityService(
        std::shared_ptr<SessionStore> sessionStore,
        std::chrono::seconds sessionTtl,
        std::shared_ptr<data::UserRepository> userRepository,
        std::shared_ptr<data::PermissionRepository> permissionRepository);

    ServiceStatus status() const;
    AuthResult login(const LoginRequest& request);
    bool logout(const std::string& token);
    std::optional<SessionInfo> validateSession(const std::string& token) const;
    bool authenticate(const std::string& username, const std::string& password) const;
    bool hasPermission(const std::vector<std::string>& permissions, const std::string& required) const;
    std::vector<std::string> permissionsForRoles(const std::vector<std::string>& roles) const;

private:
    std::string issueToken(const std::string& username);

    std::shared_ptr<SessionStore> sessionStore_;
    std::chrono::seconds sessionTtl_;
    std::shared_ptr<data::UserRepository> userRepository_;
    std::shared_ptr<data::PermissionRepository> permissionRepository_;
    std::size_t issuedSessions_{0};
};

}  // namespace induspilot::modules