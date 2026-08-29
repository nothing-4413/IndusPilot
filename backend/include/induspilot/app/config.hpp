#pragma once

#include <string>
#include <vector>

namespace induspilot::app {

struct DatabaseConfig {
    std::string host{"127.0.0.1"};
    int port{0};
    std::string database;
    std::string user;
    std::string password;
    std::string uri;
};

struct RedisConfig {
    std::string host{"127.0.0.1"};
    int port{6379};
    std::string password;
    int database{0};
    std::string uri{"tcp://127.0.0.1:6379"};
    std::string sessionKeyPrefix{"induspilot:session:"};
    int sessionTtlSeconds{28800};
    std::string sessionStore{"memory"};
};

struct AiConfig {
    bool enabled{false};
    std::string provider{"disabled"};
    std::string endpoint{"http://127.0.0.1:9000"};
    std::string apiKey;
    std::string authHeader{"Authorization"};
    std::string authScheme{"Bearer"};
    int timeoutMs{15000};
    int maxRetries{0};
    int maxResponseBytes{1048576};
    int maxContextItems{20};
    bool storeInteractionRecords{true};
    bool requireStructuredResponse{true};
    bool required{false};
};

struct NotificationConfig {
    bool webhookEnabled{false};
    int webhookTimeoutMs{5000};
    std::string webhookAllowedHosts;
};

struct ReadinessConfig {
    int probeTimeoutMs{1000};
    int probeCacheMs{1000};
};

struct ShutdownConfig {
    int drainTimeoutMs{10000};
};

struct SecurityConfig {
    bool loginLockoutEnabled{true};
    std::string loginRateLimitStore{"memory"};
    bool allowSeedCredentials{false};
    int loginMaxFailures{5};
    int loginFailureWindowSeconds{60};
    int loginLockoutSeconds{900};
    int passwordMinLength{12};
    int passwordIterations{120000};
};

struct StorageConfig {
    std::string repositoryStore{"memory"};
};

struct AppConfig {
    std::string host{"0.0.0.0"};
    int port{8080};
    std::string logLevel{"info"};
    DatabaseConfig mysql{"127.0.0.1", 3306, "induspilot", "induspilot", "", ""};
    RedisConfig redis{};
    DatabaseConfig mongodb{"127.0.0.1", 27017, "induspilot", "", "", "mongodb://127.0.0.1:27017"};
    AiConfig ai{};
    NotificationConfig notifications{};
    ReadinessConfig readiness{};
    ShutdownConfig shutdown{};
    SecurityConfig security{};
    StorageConfig storage{};
    std::vector<std::string> loadErrors;
};

struct ConfigValidation {
    bool valid{true};
    std::vector<std::string> errors;
};

AppConfig loadConfig(const std::string& path);
ConfigValidation validateConfig(const AppConfig& config);

}  // namespace induspilot::app
