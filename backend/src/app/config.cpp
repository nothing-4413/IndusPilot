#include "induspilot/app/config.hpp"

#include <charconv>
#include <cstdlib>
#include <fstream>
#include <system_error>
#include <string>

namespace induspilot::app {
namespace {

std::string trim(const std::string& value) {
    const auto begin = value.find_first_not_of(" \t\r\n\"");
    const auto end = value.find_last_not_of(" \t\r\n\"");
    if (begin == std::string::npos || end == std::string::npos) {
        return {};
    }
    return value.substr(begin, end - begin + 1);
}

bool parseInt(const std::string& value, int& parsed) {
    if (value.empty()) {
        return false;
    }
    const auto* begin = value.data();
    const auto* end = begin + value.size();
    const auto result = std::from_chars(begin, end, parsed);
    return result.ec == std::errc{} && result.ptr == end;
}

bool parseBool(const std::string& value, bool& parsed) {
    if (value == "true" || value == "1" || value == "yes" || value == "on") {
        parsed = true;
        return true;
    }
    if (value == "false" || value == "0" || value == "no" || value == "off") {
        parsed = false;
        return true;
    }
    return false;
}

void addLoadError(AppConfig& config, const std::string& message) {
    config.loadErrors.push_back(message);
}

void applyStringEnv(const char* name, std::string& target) {
    const auto* value = std::getenv(name);
    if (value != nullptr) {
        target = value;
    }
}

void applyIntEnv(AppConfig& config, const char* name, const char* field, int& target) {
    const auto* value = std::getenv(name);
    if (value == nullptr) {
        return;
    }
    int parsed = target;
    if (!parseInt(value, parsed)) {
        addLoadError(config, std::string("environment variable ") + name + " for " + field + " must be an integer");
        return;
    }
    target = parsed;
}

void applyBoolEnv(AppConfig& config, const char* name, const char* field, bool& target) {
    const auto* value = std::getenv(name);
    if (value == nullptr) {
        return;
    }
    bool parsed = target;
    if (!parseBool(value, parsed)) {
        addLoadError(config, std::string("environment variable ") + name + " for " + field + " must be a boolean");
        return;
    }
    target = parsed;
}

void refreshRedisUri(RedisConfig& redis) {
    if (!redis.uri.empty() && redis.uri != "tcp://127.0.0.1:6379") {
        return;
    }
    redis.uri = "tcp://" + redis.host + ':' + std::to_string(redis.port);
}

void applyConfigValue(
    AppConfig& config,
    const std::string& section,
    const std::string& key,
    const std::string& value,
    std::size_t lineNumber) {
    const auto field = section + "." + key;
    const auto parseInteger = [&](int& target) {
        int parsed = target;
        if (!parseInt(value, parsed)) {
            addLoadError(config, "line " + std::to_string(lineNumber) + ": " + field + " must be an integer");
            return;
        }
        target = parsed;
    };
    const auto parseBoolean = [&](bool& target) {
        bool parsed = target;
        if (!parseBool(value, parsed)) {
            addLoadError(config, "line " + std::to_string(lineNumber) + ": " + field + " must be a boolean");
            return;
        }
        target = parsed;
    };

    if (section == "server" && key == "host") {
        config.host = value;
    } else if (section == "server" && key == "port") {
        parseInteger(config.port);
    } else if (section == "log" && key == "level") {
        config.logLevel = value;
    } else if (section == "mysql" && key == "host") {
        config.mysql.host = value;
    } else if (section == "mysql" && key == "port") {
        parseInteger(config.mysql.port);
    } else if (section == "mysql" && key == "database") {
        config.mysql.database = value;
    } else if (section == "mysql" && key == "user") {
        config.mysql.user = value;
    } else if (section == "mysql" && key == "password") {
        config.mysql.password = value;
    } else if (section == "mysql" && key == "uri") {
        config.mysql.uri = value;
    } else if (section == "redis" && key == "host") {
        config.redis.host = value;
    } else if (section == "redis" && key == "port") {
        parseInteger(config.redis.port);
    } else if (section == "redis" && key == "password") {
        config.redis.password = value;
    } else if (section == "redis" && key == "database") {
        parseInteger(config.redis.database);
    } else if (section == "redis" && key == "uri") {
        config.redis.uri = value;
    } else if (section == "redis" && key == "session_key_prefix") {
        config.redis.sessionKeyPrefix = value;
    } else if (section == "redis" && key == "session_ttl_seconds") {
        parseInteger(config.redis.sessionTtlSeconds);
    } else if (section == "redis" && key == "session_store") {
        config.redis.sessionStore = value;
    } else if (section == "mongodb" && key == "host") {
        config.mongodb.host = value;
    } else if (section == "mongodb" && key == "port") {
        parseInteger(config.mongodb.port);
    } else if (section == "mongodb" && key == "database") {
        config.mongodb.database = value;
    } else if (section == "mongodb" && key == "uri") {
        config.mongodb.uri = value;
    } else if (section == "ai" && key == "enabled") {
        parseBoolean(config.ai.enabled);
    } else if (section == "ai" && key == "required") {
        parseBoolean(config.ai.required);
    } else if (section == "ai" && key == "provider") {
        config.ai.provider = value;
    } else if (section == "ai" && key == "endpoint") {
        config.ai.endpoint = value;
    } else if (section == "ai" && (key == "apiKey" || key == "api_key")) {
        config.ai.apiKey = value;
    } else if (section == "ai" && (key == "authHeader" || key == "auth_header")) {
        config.ai.authHeader = value;
    } else if (section == "ai" && (key == "authScheme" || key == "auth_scheme")) {
        config.ai.authScheme = value;
    } else if (section == "ai" && (key == "timeoutMs" || key == "timeout_ms")) {
        parseInteger(config.ai.timeoutMs);
    } else if (section == "ai" && (key == "maxRetries" || key == "max_retries")) {
        parseInteger(config.ai.maxRetries);
    } else if (section == "ai" && (key == "maxResponseBytes" || key == "max_response_bytes")) {
        parseInteger(config.ai.maxResponseBytes);
    } else if (section == "ai" && (key == "maxContextItems" || key == "max_context_items")) {
        parseInteger(config.ai.maxContextItems);
    } else if (section == "ai" && (key == "storeInteractionRecords" || key == "store_interaction_records")) {
        parseBoolean(config.ai.storeInteractionRecords);
    } else if (section == "ai" && (key == "requireStructuredResponse" || key == "require_structured_response")) {
        parseBoolean(config.ai.requireStructuredResponse);
    } else if (section == "readiness" && (key == "probeTimeoutMs" || key == "probe_timeout_ms")) {
        parseInteger(config.readiness.probeTimeoutMs);
    } else if (section == "readiness" && (key == "probeCacheMs" || key == "probe_cache_ms")) {
        parseInteger(config.readiness.probeCacheMs);
    } else if (section == "shutdown" && (key == "drainTimeoutMs" || key == "drain_timeout_ms")) {
        parseInteger(config.shutdown.drainTimeoutMs);
    } else if (section == "security" && (key == "loginLockoutEnabled" || key == "login_lockout_enabled")) {
        parseBoolean(config.security.loginLockoutEnabled);
    } else if (section == "security" && (key == "loginMaxFailures" || key == "login_max_failures")) {
        parseInteger(config.security.loginMaxFailures);
    } else if (section == "security" && (key == "loginFailureWindowSeconds" || key == "login_failure_window_seconds")) {
        parseInteger(config.security.loginFailureWindowSeconds);
    } else if (section == "security" && (key == "loginLockoutSeconds" || key == "login_lockout_seconds")) {
        parseInteger(config.security.loginLockoutSeconds);
    } else if (section == "security" && (key == "passwordMinLength" || key == "password_min_length")) {
        parseInteger(config.security.passwordMinLength);
    } else if (section == "security" && (key == "passwordIterations" || key == "password_iterations")) {
        parseInteger(config.security.passwordIterations);
    } else if (section == "storage" && key == "repository_store") {
        config.storage.repositoryStore = value;
    } else {
        addLoadError(config, "line " + std::to_string(lineNumber) + ": unknown configuration field " + field);
    }
}

void applyEnvironmentOverrides(AppConfig& config) {
    applyStringEnv("INDUSPILOT_SERVER_HOST", config.host);
    applyIntEnv(config, "INDUSPILOT_SERVER_PORT", "server.port", config.port);
    applyStringEnv("INDUSPILOT_LOG_LEVEL", config.logLevel);

    applyStringEnv("INDUSPILOT_MYSQL_HOST", config.mysql.host);
    applyIntEnv(config, "INDUSPILOT_MYSQL_PORT", "mysql.port", config.mysql.port);
    applyStringEnv("INDUSPILOT_MYSQL_DATABASE", config.mysql.database);
    applyStringEnv("INDUSPILOT_MYSQL_USER", config.mysql.user);
    applyStringEnv("INDUSPILOT_MYSQL_PASSWORD", config.mysql.password);
    applyStringEnv("INDUSPILOT_MYSQL_URI", config.mysql.uri);

    applyStringEnv("INDUSPILOT_REDIS_HOST", config.redis.host);
    applyIntEnv(config, "INDUSPILOT_REDIS_PORT", "redis.port", config.redis.port);
    applyStringEnv("INDUSPILOT_REDIS_PASSWORD", config.redis.password);
    applyIntEnv(config, "INDUSPILOT_REDIS_DATABASE", "redis.database", config.redis.database);
    applyStringEnv("INDUSPILOT_REDIS_URI", config.redis.uri);
    applyStringEnv("INDUSPILOT_REDIS_SESSION_KEY_PREFIX", config.redis.sessionKeyPrefix);
    applyIntEnv(config, "INDUSPILOT_REDIS_SESSION_TTL_SECONDS", "redis.session_ttl_seconds", config.redis.sessionTtlSeconds);
    applyStringEnv("INDUSPILOT_REDIS_SESSION_STORE", config.redis.sessionStore);

    applyStringEnv("INDUSPILOT_MONGODB_HOST", config.mongodb.host);
    applyIntEnv(config, "INDUSPILOT_MONGODB_PORT", "mongodb.port", config.mongodb.port);
    applyStringEnv("INDUSPILOT_MONGODB_DATABASE", config.mongodb.database);
    applyStringEnv("INDUSPILOT_MONGODB_URI", config.mongodb.uri);

    applyBoolEnv(config, "INDUSPILOT_AI_ENABLED", "ai.enabled", config.ai.enabled);
    applyBoolEnv(config, "INDUSPILOT_AI_REQUIRED", "ai.required", config.ai.required);
    applyStringEnv("INDUSPILOT_AI_PROVIDER", config.ai.provider);
    applyStringEnv("INDUSPILOT_AI_ENDPOINT", config.ai.endpoint);
    applyStringEnv("INDUSPILOT_AI_API_KEY", config.ai.apiKey);
    applyStringEnv("INDUSPILOT_AI_AUTH_HEADER", config.ai.authHeader);
    applyStringEnv("INDUSPILOT_AI_AUTH_SCHEME", config.ai.authScheme);
    applyIntEnv(config, "INDUSPILOT_AI_TIMEOUT_MS", "ai.timeout_ms", config.ai.timeoutMs);
    applyIntEnv(config, "INDUSPILOT_AI_MAX_RETRIES", "ai.max_retries", config.ai.maxRetries);
    applyIntEnv(config, "INDUSPILOT_AI_MAX_RESPONSE_BYTES", "ai.max_response_bytes", config.ai.maxResponseBytes);
    applyIntEnv(config, "INDUSPILOT_AI_MAX_CONTEXT_ITEMS", "ai.max_context_items", config.ai.maxContextItems);
    applyBoolEnv(config, "INDUSPILOT_AI_STORE_INTERACTION_RECORDS", "ai.store_interaction_records", config.ai.storeInteractionRecords);
    applyBoolEnv(config, "INDUSPILOT_AI_REQUIRE_STRUCTURED_RESPONSE", "ai.require_structured_response", config.ai.requireStructuredResponse);

    applyIntEnv(config, "INDUSPILOT_READINESS_PROBE_TIMEOUT_MS", "readiness.probe_timeout_ms", config.readiness.probeTimeoutMs);
    applyIntEnv(config, "INDUSPILOT_READINESS_PROBE_CACHE_MS", "readiness.probe_cache_ms", config.readiness.probeCacheMs);
    applyIntEnv(config, "INDUSPILOT_SHUTDOWN_DRAIN_TIMEOUT_MS", "shutdown.drain_timeout_ms", config.shutdown.drainTimeoutMs);

    applyBoolEnv(config, "INDUSPILOT_SECURITY_LOGIN_LOCKOUT_ENABLED", "security.login_lockout_enabled", config.security.loginLockoutEnabled);
    applyIntEnv(config, "INDUSPILOT_SECURITY_LOGIN_MAX_FAILURES", "security.login_max_failures", config.security.loginMaxFailures);
    applyIntEnv(config, "INDUSPILOT_SECURITY_LOGIN_FAILURE_WINDOW_SECONDS", "security.login_failure_window_seconds", config.security.loginFailureWindowSeconds);
    applyIntEnv(config, "INDUSPILOT_SECURITY_LOGIN_LOCKOUT_SECONDS", "security.login_lockout_seconds", config.security.loginLockoutSeconds);
    applyIntEnv(config, "INDUSPILOT_SECURITY_PASSWORD_MIN_LENGTH", "security.password_min_length", config.security.passwordMinLength);
    applyIntEnv(config, "INDUSPILOT_SECURITY_PASSWORD_ITERATIONS", "security.password_iterations", config.security.passwordIterations);

    applyStringEnv("INDUSPILOT_REPOSITORY_STORE", config.storage.repositoryStore);
}

}  // namespace

AppConfig loadConfig(const std::string& path) {
    AppConfig config;
    std::ifstream input(path);
    if (!input) {
        addLoadError(config, "configuration file could not be read: " + path);
    } else {
        std::string section;
        std::string line;
        std::size_t lineNumber = 0;
        while (std::getline(input, line)) {
            ++lineNumber;
            const auto trimmed = trim(line);
            if (trimmed.empty() || trimmed[0] == '#') {
                continue;
            }

            const auto pos = line.find(':');
            if (pos == std::string::npos) {
                addLoadError(config, "line " + std::to_string(lineNumber) + ": expected a key/value entry");
                continue;
            }

            const auto indentation = line.find_first_not_of(" \t");
            const bool isSection = indentation == 0 && trim(line.substr(pos + 1)).empty();
            if (isSection) {
                section = trim(line.substr(0, pos));
                if (section != "server" && section != "log" && section != "mysql" && section != "redis" &&
                    section != "storage" && section != "mongodb" && section != "security" && section != "ai" &&
                    section != "readiness" && section != "shutdown") {
                    addLoadError(config, "line " + std::to_string(lineNumber) + ": unknown configuration section " + section);
                }
                continue;
            }

            if (indentation == 0 || section.empty()) {
                addLoadError(config, "line " + std::to_string(lineNumber) + ": configuration field must be nested under a section");
                continue;
            }

            const auto key = trim(line.substr(0, pos));
            const auto value = trim(line.substr(pos + 1));
            applyConfigValue(config, section, key, value, lineNumber);
        }
    }

    refreshRedisUri(config.redis);
    applyEnvironmentOverrides(config);
    refreshRedisUri(config.redis);
    return config;
}

ConfigValidation validateConfig(const AppConfig& config) {
    ConfigValidation result;
    const auto addError = [&result](const std::string& message) {
        result.valid = false;
        result.errors.push_back(message);
    };

    for (const auto& loadError : config.loadErrors) {
        addError(loadError);
    }

    if (config.host.empty()) {
        addError("server.host must not be empty");
    }
    if (config.port < 1 || config.port > 65535) {
        addError("server.port must be between 1 and 65535");
    }
    if (config.storage.repositoryStore != "memory" && config.storage.repositoryStore != "mysql") {
        addError("storage.repository_store must be memory or mysql");
    }
    if (config.redis.sessionStore != "memory" && config.redis.sessionStore != "redis") {
        addError("redis.session_store must be memory or redis");
    }
    if (config.redis.sessionStore == "redis") {
        if (config.redis.uri.empty()) {
            addError("redis.uri must not be empty when redis.session_store=redis");
        }
    }
    if (config.ai.required && (!config.ai.enabled || config.ai.provider != "http")) {
        addError("ai.required requires ai.enabled=true and ai.provider=http");
    }
    if (config.ai.enabled && config.ai.provider != "disabled" && config.ai.provider != "http") {
        addError("ai.provider must be disabled or http");
    }
    if (config.ai.enabled && config.ai.provider == "http" && config.ai.endpoint.empty()) {
        addError("ai.endpoint must not be empty for the http AI provider");
    }
    if (config.ai.timeoutMs < 1) {
        addError("ai.timeout_ms must be greater than zero");
    }
    if (config.ai.maxRetries < 0) {
        addError("ai.max_retries must not be negative");
    }
    if (config.ai.maxResponseBytes < 1) {
        addError("ai.max_response_bytes must be greater than zero");
    }
    if (config.ai.maxContextItems < 1) {
        addError("ai.max_context_items must be greater than zero");
    }
    if (config.readiness.probeTimeoutMs < 1) {
        addError("readiness.probe_timeout_ms must be greater than zero");
    }
    if (config.readiness.probeCacheMs < 0) {
        addError("readiness.probe_cache_ms must not be negative");
    }
    if (config.shutdown.drainTimeoutMs < 1) {
        addError("shutdown.drain_timeout_ms must be greater than zero");
    }
    if (config.security.passwordMinLength < 8 || config.security.passwordMinLength > 1024) {
        addError("security.password_min_length must be between 8 and 1024");
    }
    if (config.security.passwordIterations < 100000 || config.security.passwordIterations > 1000000) {
        addError("security.password_iterations must be between 100000 and 1000000");
    }
    return result;
}

}  // namespace induspilot::app
