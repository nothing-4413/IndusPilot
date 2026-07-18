#pragma once

#include "induspilot/domain/domain_types.hpp"
#include "induspilot/modules/service_status.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace induspilot::modules {

struct LoginRequest {
    std::string username;
    std::string password;
};

struct SessionInfo {
    std::string token;
    domain::User user;
    bool active{true};
};

struct AuthResult {
    bool success{false};
    std::string message;
    std::optional<SessionInfo> session;
};

class IdentityService {
public:
    IdentityService();

    ServiceStatus status() const;
    AuthResult login(const LoginRequest& request);
    bool logout(const std::string& token);
    std::optional<SessionInfo> validateSession(const std::string& token) const;
    bool authenticate(const std::string& username, const std::string& password) const;
    bool hasPermission(const std::vector<std::string>& permissions, const std::string& required) const;
    std::vector<std::string> permissionsForRoles(const std::vector<std::string>& roles) const;

private:
    std::string issueToken(const std::string& username) const;

    std::map<std::string, std::string> passwordHashes_;
    std::map<std::string, domain::User> users_;
    std::map<std::string, std::vector<std::string>> rolePermissions_;
    std::map<std::string, SessionInfo> sessions_;
};

}  // namespace induspilot::modules