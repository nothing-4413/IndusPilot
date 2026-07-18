#include "induspilot/app/config.hpp"

#include <cstdlib>
#include <fstream>
#include <map>
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

std::string envValue(const char* name) {
    const auto* value = std::getenv(name);
    if (value == nullptr) {
        return {};
    }
    return value;
}

int parseInt(const std::string& value, int fallback) {
    if (value.empty()) {
        return fallback;
    }
    try {
        return std::stoi(value);
    } catch (...) {
        return fallback;
    }
}

bool parseBool(const std::string& value, bool fallback) {
    if (value == "true" || value == "1" || value == "yes" || value == "on") {
        return true;
    }
    if (value == "false" || value == "0" || value == "no" || value == "off") {
        return false;
    }
    return fallback;
}

void applyStringEnv(const char* name, std::string& target) {
    const auto value = envValue(name);
    if (!value.empty()) {
        target = value;
    }
}

void applyIntEnv(const char* name, int& target) {
    const auto value = envValue(name);
    if (!value.empty()) {
        target = parseInt(value, target);
    }
}

void applyBoolEnv(const char* name, bool& target) {
    const auto value = envValue(name);
    if (!value.empty()) {
        target = parseBool(value, target);
    }
}

void refreshRedisUri(RedisConfig& redis) {
    if (!redis.uri.empty() && redis.uri != "tcp://127.0.0.1:6379") {
        return;
    }
    redis.uri = "tcp://" + redis.host + ':' + std::to_string(redis.port);
}

void applyConfigValue(AppConfig& config, const std::string& section, const std::string& key, const std::string& value) {
    if (section == "server" && key == "host") {
        config.host = value;
    } else if (section == "server" && key == "port") {
        config.port = parseInt(value, config.port);
    } else if (section == "log" && key == "level") {
        config.logLevel = value;
    } else if (section == "mysql" && key == "host") {
        config.mysql.host = value;
    } else if (section == "mysql" && key == "port") {
        config.mysql.port = parseInt(value, config.mysql.port);
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
        config.redis.port = parseInt(value, config.redis.port);
    } else if (section == "redis" && key == "password") {
        config.redis.password = value;
    } else if (section == "redis" && key == "database") {
        config.redis.database = parseInt(value, config.redis.database);
    } else if (section == "redis" && key == "uri") {
        config.redis.uri = value;
    } else if (section == "redis" && key == "session_key_prefix") {
        config.redis.sessionKeyPrefix = value;
    } else if (section == "redis" && key == "session_ttl_seconds") {
        config.redis.sessionTtlSeconds = parseInt(value, config.redis.sessionTtlSeconds);
    } else if (section == "redis" && key == "session_store") {
        config.redis.sessionStore = value;
    } else if (section == "mongodb" && key == "host") {
        config.mongodb.host = value;
    } else if (section == "mongodb" && key == "port") {
        config.mongodb.port = parseInt(value, config.mongodb.port);
    } else if (section == "mongodb" && key == "database") {
        config.mongodb.database = value;
    } else if (section == "mongodb" && key == "uri") {
        config.mongodb.uri = value;
    } else if (section == "ai" && key == "enabled") {
        config.ai.enabled = parseBool(value, config.ai.enabled);
    } else if (section == "ai" && key == "endpoint") {
        config.ai.endpoint = value;
    }
}

void applyEnvironmentOverrides(AppConfig& config) {
    applyStringEnv("INDUSPILOT_SERVER_HOST", config.host);
    applyIntEnv("INDUSPILOT_SERVER_PORT", config.port);
    applyStringEnv("INDUSPILOT_LOG_LEVEL", config.logLevel);

    applyStringEnv("INDUSPILOT_MYSQL_HOST", config.mysql.host);
    applyIntEnv("INDUSPILOT_MYSQL_PORT", config.mysql.port);
    applyStringEnv("INDUSPILOT_MYSQL_DATABASE", config.mysql.database);
    applyStringEnv("INDUSPILOT_MYSQL_USER", config.mysql.user);
    applyStringEnv("INDUSPILOT_MYSQL_PASSWORD", config.mysql.password);
    applyStringEnv("INDUSPILOT_MYSQL_URI", config.mysql.uri);

    applyStringEnv("INDUSPILOT_REDIS_HOST", config.redis.host);
    applyIntEnv("INDUSPILOT_REDIS_PORT", config.redis.port);
    applyStringEnv("INDUSPILOT_REDIS_PASSWORD", config.redis.password);
    applyIntEnv("INDUSPILOT_REDIS_DATABASE", config.redis.database);
    applyStringEnv("INDUSPILOT_REDIS_URI", config.redis.uri);
    applyStringEnv("INDUSPILOT_REDIS_SESSION_KEY_PREFIX", config.redis.sessionKeyPrefix);
    applyIntEnv("INDUSPILOT_REDIS_SESSION_TTL_SECONDS", config.redis.sessionTtlSeconds);
    applyStringEnv("INDUSPILOT_REDIS_SESSION_STORE", config.redis.sessionStore);

    applyStringEnv("INDUSPILOT_MONGODB_HOST", config.mongodb.host);
    applyIntEnv("INDUSPILOT_MONGODB_PORT", config.mongodb.port);
    applyStringEnv("INDUSPILOT_MONGODB_DATABASE", config.mongodb.database);
    applyStringEnv("INDUSPILOT_MONGODB_URI", config.mongodb.uri);

    applyBoolEnv("INDUSPILOT_AI_ENABLED", config.ai.enabled);
    applyStringEnv("INDUSPILOT_AI_ENDPOINT", config.ai.endpoint);
}

}  // namespace

AppConfig loadConfig(const std::string& path) {
    AppConfig config;
    std::ifstream input(path);
    if (input) {
        std::string section;
        std::string line;
        while (std::getline(input, line)) {
            const auto trimmed = trim(line);
            if (trimmed.empty() || trimmed[0] == '#') {
                continue;
            }

            const auto pos = line.find(':');
            if (pos == std::string::npos) {
                continue;
            }

            const bool isSection = line.find_first_not_of(" \t") == 0 && trim(line.substr(pos + 1)).empty();
            if (isSection) {
                section = trim(line.substr(0, pos));
                continue;
            }

            const auto key = trim(line.substr(0, pos));
            const auto value = trim(line.substr(pos + 1));
            applyConfigValue(config, section, key, value);
        }
    }

    refreshRedisUri(config.redis);
    applyEnvironmentOverrides(config);
    refreshRedisUri(config.redis);
    return config;
}

}  // namespace induspilot::app