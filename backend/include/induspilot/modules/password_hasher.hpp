#pragma once

#include <string>

namespace induspilot::modules {

bool verifyPassword(const std::string& password, const std::string& storedHash);
std::string pbkdf2Sha256PasswordHash(const std::string& password, const std::string& salt, int iterations);

}  // namespace induspilot::modules