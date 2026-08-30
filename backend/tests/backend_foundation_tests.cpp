#include "induspilot/api/router.hpp"
#include "induspilot/app/application.hpp"
#include "induspilot/app/config.hpp"
#include "induspilot/data/repositories.hpp"
#include "induspilot/data/in_memory_repositories.hpp"
#ifdef INDUSPILOT_WITH_DROGON
#include "induspilot/data/mysql_repositories.hpp"
#endif
#ifdef INDUSPILOT_WITH_MONGODB
#include "induspilot/data/mongodb_repositories.hpp"
#endif
#include "induspilot/modules/ai_service.hpp"
#include "induspilot/modules/alert_service.hpp"
#include "induspilot/modules/audit_service.hpp"
#include "induspilot/modules/asset_service.hpp"
#include "induspilot/modules/identity_service.hpp"
#include "induspilot/modules/maintenance_service.hpp"
#include "induspilot/modules/metrics_service.hpp"
#include "induspilot/modules/monitoring_service.hpp"
#include "induspilot/modules/password_hasher.hpp"
#include "induspilot/http/http_request_lifecycle.hpp"

#include <cassert>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <thread>

class RecordingAuditDeliverySink final : public induspilot::modules::AuditDeliverySink {
public:
    induspilot::modules::AuditDeliveryResult deliver(const induspilot::domain::OperationAuditEvent&) const override {
        deliveryCount.fetch_add(1);
        return {true, {}};
    }

    mutable std::atomic<int> deliveryCount{0};
};

class FailingAuditDeliverySink final : public induspilot::modules::AuditDeliverySink {
public:
    induspilot::modules::AuditDeliveryResult deliver(const induspilot::domain::OperationAuditEvent&) const override {
        deliveryCount.fetch_add(1);
        return {false, "simulated SIEM outage"};
    }

    mutable std::atomic<int> deliveryCount{0};
};

static_assert(std::has_virtual_destructor_v<induspilot::data::UserRepository>);
static_assert(std::has_virtual_destructor_v<induspilot::data::AssetRepository>);
static_assert(std::has_virtual_destructor_v<induspilot::data::AlertRepository>);
static_assert(std::has_virtual_destructor_v<induspilot::data::WorkOrderRepository>);
static_assert(std::has_virtual_destructor_v<induspilot::data::AiInteractionRepository>);
static_assert(std::has_virtual_destructor_v<induspilot::data::OperationAuditRepository>);
static_assert(std::has_virtual_destructor_v<induspilot::data::AuditDeliveryQueueRepository>);
#ifdef INDUSPILOT_WITH_DROGON
static_assert(std::is_base_of_v<induspilot::data::UserRepository, induspilot::data::MySqlUserRepository>);
static_assert(std::is_base_of_v<induspilot::data::AssetRepository, induspilot::data::MySqlAssetRepository>);
static_assert(std::is_base_of_v<induspilot::data::AlertRepository, induspilot::data::MySqlAlertRepository>);
static_assert(std::is_base_of_v<induspilot::data::WorkOrderRepository, induspilot::data::MySqlWorkOrderRepository>);
static_assert(std::is_base_of_v<induspilot::data::RuntimeStateRepository, induspilot::data::MySqlRuntimeStateRepository>);
static_assert(std::is_base_of_v<induspilot::data::AuditDeliveryQueueRepository, induspilot::data::MySqlAuditDeliveryQueueRepository>);
static_assert(std::is_base_of_v<induspilot::data::AiInteractionRepository, induspilot::data::MySqlAiInteractionRepository>);
#endif
#ifdef INDUSPILOT_WITH_MONGODB
static_assert(std::is_base_of_v<induspilot::data::AiInteractionRepository, induspilot::data::MongoAiInteractionRepository>);
#endif

class FailingPasswordUpdateUserRepository final : public induspilot::data::UserRepository {
public:
    std::optional<induspilot::data::UserCredential> findByUsername(const std::string& username) const override {
        if (username != "admin") {
            return std::nullopt;
        }
        return induspilot::data::UserCredential{
            induspilot::domain::User{"user-admin", "admin", {"admin"}},
            "plain:admin123"};
    }

    std::vector<induspilot::domain::User> listUsers() const override {
        return {induspilot::domain::User{"user-admin", "admin", {"admin"}}};
    }

    bool updatePasswordHash(const std::string&, const std::string&) override {
        return false;
    }
};

int main() {
    induspilot::http::HttpRequestLifecycle requestLifecycle;
    assert(requestLifecycle.accepting());
    assert(requestLifecycle.tryBeginRequest());
    assert(requestLifecycle.inFlightRequests() == 1);
    assert(requestLifecycle.tryBeginControlPlaneRequest());
    assert(requestLifecycle.inFlightRequests() == 2);
    requestLifecycle.stopAccepting();
    assert(!requestLifecycle.accepting());
    assert(!requestLifecycle.tryBeginRequest());
    requestLifecycle.finishRequest();
    requestLifecycle.finishRequest();
    requestLifecycle.waitForDrain();
    assert(requestLifecycle.inFlightRequests() == 0);

    induspilot::http::HttpRequestLifecycle timedRequestLifecycle;
    assert(timedRequestLifecycle.tryBeginRequest());
    timedRequestLifecycle.stopAccepting();
    assert(!timedRequestLifecycle.waitForDrainFor(std::chrono::milliseconds(10)));
    timedRequestLifecycle.finishRequest();
    assert(timedRequestLifecycle.waitForDrainFor(std::chrono::milliseconds(10)));

#ifdef _WIN32
    _putenv_s("INDUSPILOT_SERVER_PORT", "18080");
    _putenv_s("INDUSPILOT_REDIS_SESSION_TTL_SECONDS", "60");
    _putenv_s("INDUSPILOT_REDIS_SESSION_KEY_PREFIX", "test:session:");
    _putenv_s("INDUSPILOT_REDIS_SESSION_STORE", "redis");
    _putenv_s("INDUSPILOT_REPOSITORY_STORE", "mysql");
    _putenv_s("INDUSPILOT_AI_PROVIDER", "http");
    _putenv_s("INDUSPILOT_AI_REQUIRED", "true");
    _putenv_s("INDUSPILOT_AI_TIMEOUT_MS", "2500");
    _putenv_s("INDUSPILOT_AI_MAX_RETRIES", "2");
    _putenv_s("INDUSPILOT_AI_MAX_RESPONSE_BYTES", "2048");
    _putenv_s("INDUSPILOT_AI_MAX_CONTEXT_ITEMS", "3");
    _putenv_s("INDUSPILOT_AI_STORE_INTERACTION_RECORDS", "false");
    _putenv_s("INDUSPILOT_AI_API_KEY", "test-ai-key");
    _putenv_s("INDUSPILOT_AI_AUTH_HEADER", "X-Test-AI-Key");
    _putenv_s("INDUSPILOT_AI_AUTH_SCHEME", "Token");
    _putenv_s("INDUSPILOT_AI_REQUIRE_STRUCTURED_RESPONSE", "false");
    _putenv_s("INDUSPILOT_READINESS_PROBE_TIMEOUT_MS", "1200");
    _putenv_s("INDUSPILOT_READINESS_PROBE_CACHE_MS", "250");
    _putenv_s("INDUSPILOT_SHUTDOWN_DRAIN_TIMEOUT_MS", "4200");
    _putenv_s("INDUSPILOT_MYSQL_URI", "host=127.0.0.1 port=3306 dbname=induspilot user=induspilot");
    _putenv_s("INDUSPILOT_SECURITY_LOGIN_MAX_FAILURES", "3");
    _putenv_s("INDUSPILOT_SECURITY_LOGIN_LOCKOUT_SECONDS", "120");
#else
    setenv("INDUSPILOT_SERVER_PORT", "18080", 1);
    setenv("INDUSPILOT_REDIS_SESSION_TTL_SECONDS", "60", 1);
    setenv("INDUSPILOT_REDIS_SESSION_KEY_PREFIX", "test:session:", 1);
    setenv("INDUSPILOT_REDIS_SESSION_STORE", "redis", 1);
    setenv("INDUSPILOT_REPOSITORY_STORE", "mysql", 1);
    setenv("INDUSPILOT_AI_PROVIDER", "http", 1);
    setenv("INDUSPILOT_AI_REQUIRED", "true", 1);
    setenv("INDUSPILOT_AI_TIMEOUT_MS", "2500", 1);
    setenv("INDUSPILOT_AI_MAX_RETRIES", "2", 1);
    setenv("INDUSPILOT_AI_MAX_RESPONSE_BYTES", "2048", 1);
    setenv("INDUSPILOT_AI_MAX_CONTEXT_ITEMS", "3", 1);
    setenv("INDUSPILOT_AI_STORE_INTERACTION_RECORDS", "false", 1);
    setenv("INDUSPILOT_AI_API_KEY", "test-ai-key", 1);
    setenv("INDUSPILOT_AI_AUTH_HEADER", "X-Test-AI-Key", 1);
    setenv("INDUSPILOT_AI_AUTH_SCHEME", "Token", 1);
    setenv("INDUSPILOT_AI_REQUIRE_STRUCTURED_RESPONSE", "false", 1);
    setenv("INDUSPILOT_READINESS_PROBE_TIMEOUT_MS", "1200", 1);
    setenv("INDUSPILOT_READINESS_PROBE_CACHE_MS", "250", 1);
    setenv("INDUSPILOT_SHUTDOWN_DRAIN_TIMEOUT_MS", "4200", 1);
    setenv("INDUSPILOT_MYSQL_URI", "host=127.0.0.1 port=3306 dbname=induspilot user=induspilot", 1);
    setenv("INDUSPILOT_SECURITY_LOGIN_MAX_FAILURES", "3", 1);
    setenv("INDUSPILOT_SECURITY_LOGIN_LOCKOUT_SECONDS", "120", 1);
#endif
    const auto loadedConfig = induspilot::app::loadConfig("config/backend.example.yaml");
    assert(loadedConfig.port == 18080);
    assert(loadedConfig.redis.sessionTtlSeconds == 60);
    assert(loadedConfig.redis.sessionKeyPrefix == "test:session:");
    assert(loadedConfig.redis.sessionStore == "redis");
    assert(loadedConfig.storage.repositoryStore == "mysql");
    assert(loadedConfig.ai.provider == "http");
    assert(loadedConfig.ai.required);
    assert(loadedConfig.ai.timeoutMs == 2500);
    assert(loadedConfig.ai.maxRetries == 2);
    assert(loadedConfig.ai.maxResponseBytes == 2048);
    assert(loadedConfig.ai.maxContextItems == 3);
    assert(!loadedConfig.ai.storeInteractionRecords);
    assert(loadedConfig.ai.apiKey == "test-ai-key");
    assert(loadedConfig.ai.authHeader == "X-Test-AI-Key");
    assert(loadedConfig.ai.authScheme == "Token");
    assert(!loadedConfig.ai.requireStructuredResponse);
    assert(loadedConfig.readiness.probeTimeoutMs == 1200);
    assert(loadedConfig.readiness.probeCacheMs == 250);
    assert(loadedConfig.shutdown.drainTimeoutMs == 4200);
    assert(loadedConfig.mysql.uri == "host=127.0.0.1 port=3306 dbname=induspilot user=induspilot");
    assert(loadedConfig.security.loginMaxFailures == 3);
    assert(loadedConfig.security.loginRateLimitStore == "memory");
    assert(!loadedConfig.security.productionMode);
    assert(loadedConfig.security.allowSeedCredentials);
    assert(loadedConfig.security.loginLockoutSeconds == 120);
    assert(!loadedConfig.notifications.webhookEnabled);
    assert(loadedConfig.notifications.webhookTimeoutMs == 5000);
    assert(loadedConfig.notifications.webhookAllowedHosts.empty());
    auto invalidWebhookConfig = induspilot::app::AppConfig{};
    invalidWebhookConfig.notifications.webhookEnabled = true;
    assert(!induspilot::app::validateConfig(invalidWebhookConfig).valid);
    auto invalidSiemConfig = induspilot::app::AppConfig{};
    invalidSiemConfig.audit.siemWebhookEnabled = true;
    assert(!induspilot::app::validateConfig(invalidSiemConfig).valid);
    invalidSiemConfig.audit.siemWebhookUrl = "ftp://siem.example.test/events";
    invalidSiemConfig.audit.siemWebhookAllowedHosts = "siem.example.test";
    assert(!induspilot::app::validateConfig(invalidSiemConfig).valid);
    invalidSiemConfig.audit.siemWebhookUrl = "https://siem.example.test/events";
    invalidSiemConfig.audit.siemWebhookAllowedHosts = "siem.example.test";
    assert(induspilot::app::validateConfig(invalidSiemConfig).valid);
    invalidSiemConfig.audit.siemWebhookMaxAttempts = 0;
    assert(!induspilot::app::validateConfig(invalidSiemConfig).valid);
    invalidSiemConfig.audit.siemWebhookMaxAttempts = 3;
    invalidSiemConfig.audit.siemWebhookPollMs = 9;
    assert(!induspilot::app::validateConfig(invalidSiemConfig).valid);
    invalidSiemConfig.audit.siemWebhookPollMs = 250;
    assert(induspilot::app::validateConfig(invalidSiemConfig).valid);
    assert(loadedConfig.security.passwordMinLength == 12);
    assert(loadedConfig.security.passwordIterations == 120000);
    assert(induspilot::app::validateConfig(induspilot::app::AppConfig{}).valid);

#ifdef _WIN32
    _putenv_s("INDUSPILOT_READINESS_PROBE_TIMEOUT_MS", "invalid");
#else
    setenv("INDUSPILOT_READINESS_PROBE_TIMEOUT_MS", "invalid", 1);
#endif
    const auto invalidEnvironmentConfig = induspilot::app::loadConfig("config/backend.example.yaml");
    assert(!invalidEnvironmentConfig.loadErrors.empty());
    assert(!induspilot::app::validateConfig(invalidEnvironmentConfig).valid);
#ifdef _WIN32
    _putenv_s("INDUSPILOT_READINESS_PROBE_TIMEOUT_MS", "1200");
#else
    setenv("INDUSPILOT_READINESS_PROBE_TIMEOUT_MS", "1200", 1);
#endif

    const auto invalidConfigPath = std::filesystem::temp_directory_path() /
        ("induspilot-invalid-config-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".yaml");
    {
        std::ofstream invalidConfigFile(invalidConfigPath);
        invalidConfigFile << "server:\n"
                          << "  port: not-an-integer\n"
                          << "  unexpected: true\n"
                          << "broken line\n";
    }
    const auto invalidFileConfig = induspilot::app::loadConfig(invalidConfigPath.string());
    assert(invalidFileConfig.loadErrors.size() >= 3);
    const auto invalidFileValidation = induspilot::app::validateConfig(invalidFileConfig);
    assert(!invalidFileValidation.valid);
    induspilot::app::Application invalidLoadedApplication(invalidFileConfig);
    assert(!invalidLoadedApplication.start());
    assert(!invalidLoadedApplication.startup().configurationValid);
    std::filesystem::remove(invalidConfigPath);

    const auto missingConfig = induspilot::app::loadConfig("config/does-not-exist.yaml");
    assert(!missingConfig.loadErrors.empty());
    assert(!induspilot::app::validateConfig(missingConfig).valid);

    auto invalidConfig = induspilot::app::AppConfig{};
    invalidConfig.port = 0;
    assert(!induspilot::app::validateConfig(invalidConfig).valid);
    invalidConfig = induspilot::app::AppConfig{};
    invalidConfig.storage.repositoryStore = "unknown";
    assert(!induspilot::app::validateConfig(invalidConfig).valid);
    invalidConfig = induspilot::app::AppConfig{};
    invalidConfig.ai.required = true;
    assert(!induspilot::app::validateConfig(invalidConfig).valid);
    invalidConfig = induspilot::app::AppConfig{};
    invalidConfig.ai.maxRetries = -1;
    assert(!induspilot::app::validateConfig(invalidConfig).valid);
    invalidConfig = induspilot::app::AppConfig{};
    invalidConfig.ai.maxResponseBytes = 0;
    assert(!induspilot::app::validateConfig(invalidConfig).valid);
    invalidConfig = induspilot::app::AppConfig{};
    invalidConfig.readiness.probeCacheMs = -1;
    assert(!induspilot::app::validateConfig(invalidConfig).valid);
    invalidConfig = induspilot::app::AppConfig{};
    invalidConfig.shutdown.drainTimeoutMs = 0;
    assert(!induspilot::app::validateConfig(invalidConfig).valid);
    invalidConfig = induspilot::app::AppConfig{};
    invalidConfig.security.passwordMinLength = 7;
    assert(!induspilot::app::validateConfig(invalidConfig).valid);
    invalidConfig = induspilot::app::AppConfig{};
    invalidConfig.security.passwordIterations = 99999;
    assert(!induspilot::app::validateConfig(invalidConfig).valid);
    invalidConfig = induspilot::app::AppConfig{};
    invalidConfig.security.loginRateLimitStore = "unknown";
    assert(!induspilot::app::validateConfig(invalidConfig).valid);
    invalidConfig = induspilot::app::AppConfig{};
    invalidConfig.security.productionMode = true;
    invalidConfig.security.allowSeedCredentials = true;
    assert(!induspilot::app::validateConfig(invalidConfig).valid);
    invalidConfig.security.allowSeedCredentials = false;
    invalidConfig.storage.repositoryStore = "mysql";
    invalidConfig.mysql.password = "change-me-app-password";
    assert(!induspilot::app::validateConfig(invalidConfig).valid);
    invalidConfig.mysql.password = "production-test-secret";
    assert(induspilot::app::validateConfig(invalidConfig).valid);
    invalidConfig.storage.aiInteractionStore = "unknown";
    assert(!induspilot::app::validateConfig(invalidConfig).valid);
    invalidConfig.storage.aiInteractionStore = "memory";
    assert(induspilot::app::validateConfig(invalidConfig).valid);
#ifndef INDUSPILOT_WITH_MONGODB
    invalidConfig.storage.aiInteractionStore = "mongodb";
    assert(!induspilot::app::validateConfig(invalidConfig).valid);
#endif

    const auto memoryRequirements = induspilot::data::DataConnectors{induspilot::app::AppConfig{}}.requirements();
    assert(!memoryRequirements.mysql);
    assert(!memoryRequirements.redis);
    assert(!memoryRequirements.mongodb);
    assert(!memoryRequirements.ai);
    const auto memoryStatus = induspilot::data::DataConnectors{induspilot::app::AppConfig{}}.probe();
    assert(!memoryStatus.mysql.checked);
    assert(!memoryStatus.redis.checked);
    assert(!memoryStatus.mongodb.checked);
    assert(!memoryStatus.ai.checked);
    auto mysqlConfig = induspilot::app::AppConfig{};
    mysqlConfig.storage.repositoryStore = "mysql";
    assert(induspilot::data::DataConnectors{mysqlConfig}.requirements().mysql);
    auto redisConfig = induspilot::app::AppConfig{};
    redisConfig.redis.sessionStore = "redis";
    assert(induspilot::data::DataConnectors{redisConfig}.requirements().redis);
    auto aiConfig = induspilot::app::AppConfig{};
    aiConfig.ai.enabled = true;
    aiConfig.ai.provider = "http";
    assert(induspilot::data::DataConnectors{aiConfig}.requirements().ai);
    aiConfig.ai.required = true;
    const auto aiStatus = induspilot::data::DataConnectors{aiConfig}.probe();
    assert(aiStatus.ai.required);
    auto mongodbConfig = induspilot::app::AppConfig{};
    mongodbConfig.storage.aiInteractionStore = "mongodb";
    assert(induspilot::data::DataConnectors{mongodbConfig}.requirements().mongodb);
    assert(induspilot::data::DataConnectors{mongodbConfig}.probe().mongodb.checked);
#ifdef _WIN32
    _putenv_s("INDUSPILOT_SERVER_PORT", "");
    _putenv_s("INDUSPILOT_REDIS_SESSION_TTL_SECONDS", "");
    _putenv_s("INDUSPILOT_REDIS_SESSION_KEY_PREFIX", "");
    _putenv_s("INDUSPILOT_REDIS_SESSION_STORE", "");
    _putenv_s("INDUSPILOT_REPOSITORY_STORE", "");
    _putenv_s("INDUSPILOT_AI_PROVIDER", "");
    _putenv_s("INDUSPILOT_AI_REQUIRED", "");
    _putenv_s("INDUSPILOT_AI_TIMEOUT_MS", "");
    _putenv_s("INDUSPILOT_AI_MAX_RETRIES", "");
    _putenv_s("INDUSPILOT_AI_MAX_RESPONSE_BYTES", "");
    _putenv_s("INDUSPILOT_AI_MAX_CONTEXT_ITEMS", "");
    _putenv_s("INDUSPILOT_AI_STORE_INTERACTION_RECORDS", "");
    _putenv_s("INDUSPILOT_AI_API_KEY", "");
    _putenv_s("INDUSPILOT_AI_AUTH_HEADER", "");
    _putenv_s("INDUSPILOT_AI_AUTH_SCHEME", "");
    _putenv_s("INDUSPILOT_AI_REQUIRE_STRUCTURED_RESPONSE", "");
    _putenv_s("INDUSPILOT_READINESS_PROBE_TIMEOUT_MS", "");
    _putenv_s("INDUSPILOT_READINESS_PROBE_CACHE_MS", "");
    _putenv_s("INDUSPILOT_SHUTDOWN_DRAIN_TIMEOUT_MS", "");
    _putenv_s("INDUSPILOT_MYSQL_URI", "");
    _putenv_s("INDUSPILOT_SECURITY_LOGIN_MAX_FAILURES", "");
    _putenv_s("INDUSPILOT_SECURITY_LOGIN_LOCKOUT_SECONDS", "");
#else
    unsetenv("INDUSPILOT_SERVER_PORT");
    unsetenv("INDUSPILOT_REDIS_SESSION_TTL_SECONDS");
    unsetenv("INDUSPILOT_REDIS_SESSION_KEY_PREFIX");
    unsetenv("INDUSPILOT_REDIS_SESSION_STORE");
    unsetenv("INDUSPILOT_REPOSITORY_STORE");
    unsetenv("INDUSPILOT_AI_PROVIDER");
    unsetenv("INDUSPILOT_AI_REQUIRED");
    unsetenv("INDUSPILOT_AI_TIMEOUT_MS");
    unsetenv("INDUSPILOT_AI_MAX_RETRIES");
    unsetenv("INDUSPILOT_AI_MAX_RESPONSE_BYTES");
    unsetenv("INDUSPILOT_AI_MAX_CONTEXT_ITEMS");
    unsetenv("INDUSPILOT_AI_STORE_INTERACTION_RECORDS");
    unsetenv("INDUSPILOT_AI_API_KEY");
    unsetenv("INDUSPILOT_AI_AUTH_HEADER");
    unsetenv("INDUSPILOT_AI_AUTH_SCHEME");
    unsetenv("INDUSPILOT_AI_REQUIRE_STRUCTURED_RESPONSE");
    unsetenv("INDUSPILOT_READINESS_PROBE_TIMEOUT_MS");
    unsetenv("INDUSPILOT_READINESS_PROBE_CACHE_MS");
    unsetenv("INDUSPILOT_SHUTDOWN_DRAIN_TIMEOUT_MS");
    unsetenv("INDUSPILOT_MYSQL_URI");
    unsetenv("INDUSPILOT_SECURITY_LOGIN_MAX_FAILURES");
    unsetenv("INDUSPILOT_SECURITY_LOGIN_LOCKOUT_SECONDS");
#endif

    induspilot::app::Application app(induspilot::app::AppConfig{});
    app.start();
    assert(app.isRunning());
    assert(!app.isDraining());
    assert(app.lifecycle() == induspilot::app::Application::LifecycleState::running);
    assert(app.router().handle("GET", "/health").success);
    assert(app.router().handle("GET", "/health/live").success);
    assert(app.router().handle("GET", "/health/ready").success);
    assert(app.router().handle("GET", "/health/startup").success);
    assert(!app.router().handle("GET", "/missing").success);

    app.beginDraining();
    assert(app.isRunning());
    assert(app.isDraining());
    assert(app.lifecycle() == induspilot::app::Application::LifecycleState::draining);
    assert(!app.router().handle("GET", "/health/ready").success);
    assert(app.router().handle("GET", "/health/live").success);
    app.beginDraining();
    app.stop();
    app.stop();
    assert(!app.isRunning());
    assert(!app.isDraining());
    assert(app.lifecycle() == induspilot::app::Application::LifecycleState::stopped);

    auto invalidApplicationConfig = induspilot::app::AppConfig{};
    invalidApplicationConfig.port = 0;
    induspilot::app::Application invalidApplication(invalidApplicationConfig);
    assert(!invalidApplication.start());
    assert(!invalidApplication.isRunning());
    assert(!invalidApplication.startup().configurationValid);
    assert(!invalidApplication.startup().initialized);

    auto unavailableDependencyConfig = induspilot::app::AppConfig{};
    unavailableDependencyConfig.storage.repositoryStore = "mysql";
    unavailableDependencyConfig.mysql.host = "127.0.0.1";
    unavailableDependencyConfig.mysql.port = 1;
    unavailableDependencyConfig.readiness.probeTimeoutMs = 25;
    unavailableDependencyConfig.readiness.probeCacheMs = 1000;
    induspilot::app::Application unavailableDependencyApplication(unavailableDependencyConfig);
    assert(unavailableDependencyApplication.start());
    const auto unavailableReadiness = unavailableDependencyApplication.readiness();
    assert(!unavailableReadiness.ready);
    assert(unavailableReadiness.dependencies.at("mysql").required);
    assert(unavailableReadiness.dependencies.at("mysql").checked);
    assert(!unavailableReadiness.dependencies.at("mysql").available);
    assert(unavailableReadiness.probeCount == 1);
    assert(unavailableReadiness.lastProbeDurationMs >= 0);
    assert(unavailableReadiness.lastProbeAtUnixMs > 0);

    induspilot::modules::IdentityService identity;
    const auto login = identity.login({"admin", "admin123"});
    assert(login.success);
    assert(login.session.has_value());
    assert(login.session->token.rfind("session-", 0) == 0);
    assert(login.session->token.size() == std::string_view("session-").size() + 64U);
    assert(login.session->token.find("admin") == std::string::npos);
    const auto secondLogin = identity.login({"admin", "admin123"});
    assert(secondLogin.success);
    assert(secondLogin.session.has_value());
    assert(secondLogin.session->token != login.session->token);
    assert(identity.validateSession(login.session->token).has_value());
    const auto permissions = identity.permissionsForRoles(login.session->user.roles);
    assert(identity.hasPermission(permissions, "asset:write"));
    assert(identity.hasPermission(permissions, "monitoring:write"));
    assert(identity.authenticate("admin", "admin123"));
    assert(!identity.authenticate("admin", "wrong-password"));

    induspilot::modules::PasswordPolicy passwordPolicy;
    passwordPolicy.minimumLength = 12;
    passwordPolicy.iterations = 100000;
    induspilot::modules::IdentityService passwordIdentity(
        std::make_shared<induspilot::modules::InMemorySessionStore>(),
        std::chrono::hours(8),
        std::make_shared<induspilot::data::InMemoryUserRepository>(),
        std::make_shared<induspilot::data::InMemoryPermissionRepository>(),
        {},
        passwordPolicy);
    const auto passwordSession = passwordIdentity.login({"admin", "admin123"});
    assert(passwordSession.success);
    const auto secondPasswordSession = passwordIdentity.login({"admin", "admin123"});
    assert(secondPasswordSession.success);
    assert(passwordSession.session->credentialVersion == 1);
    assert(secondPasswordSession.session->credentialVersion == passwordSession.session->credentialVersion);
    const auto shortPassword = passwordIdentity.changePassword("admin", "admin123", "short");
    assert(!shortPassword.success);
    assert(shortPassword.code == "PASSWORD_POLICY_VIOLATION");
    const auto incorrectCurrentPassword = passwordIdentity.changePassword("admin", "wrong-password", "a-long-new-password");
    assert(!incorrectCurrentPassword.success);
    assert(incorrectCurrentPassword.code == "CURRENT_PASSWORD_INVALID");
    const auto changedPassword = passwordIdentity.changePassword("admin", "admin123", "a-long-new-password");
    assert(changedPassword.success);
    assert(passwordIdentity.authenticate("admin", "a-long-new-password"));
    assert(!passwordIdentity.authenticate("admin", "admin123"));
    assert(!passwordIdentity.validateSession(passwordSession.session->token).has_value());
    assert(!passwordIdentity.validateSession(secondPasswordSession.session->token).has_value());
    const auto reLoginAfterRotation = passwordIdentity.login({"admin", "a-long-new-password"});
    assert(reLoginAfterRotation.success);
    assert(reLoginAfterRotation.session->credentialVersion == 2);
    assert(passwordIdentity.validateSession(reLoginAfterRotation.session->token).has_value());
    induspilot::modules::IdentityService storageFailureIdentity(
        std::make_shared<induspilot::modules::InMemorySessionStore>(),
        std::chrono::hours(8),
        std::make_shared<FailingPasswordUpdateUserRepository>(),
        std::make_shared<induspilot::data::InMemoryPermissionRepository>(),
        {},
        passwordPolicy);
    const auto storageFailure = storageFailureIdentity.changePassword("admin", "admin123", "a-long-new-password");
    assert(!storageFailure.success);
    assert(storageFailure.code == "PASSWORD_UPDATE_FAILED");

    induspilot::modules::LoginSecurityPolicy lockoutPolicy;
    lockoutPolicy.maxFailures = 2;
    lockoutPolicy.failureWindow = std::chrono::seconds(60);
    lockoutPolicy.lockDuration = std::chrono::seconds(120);
    induspilot::modules::IdentityService guardedIdentity(
        std::make_shared<induspilot::modules::InMemorySessionStore>(),
        std::chrono::hours(8),
        std::make_shared<induspilot::data::InMemoryUserRepository>(),
        std::make_shared<induspilot::data::InMemoryPermissionRepository>(),
        lockoutPolicy);
    const auto firstFailure = guardedIdentity.login({"admin", "wrong-password"});
    assert(!firstFailure.success);
    assert(firstFailure.code == "AUTHENTICATION_FAILED");
    const auto lockedFailure = guardedIdentity.login({"admin", "wrong-password"});
    assert(!lockedFailure.success);
    assert(lockedFailure.code == "AUTHENTICATION_LOCKED");
    assert(lockedFailure.retryAfterSeconds > 0);
    const auto lockedSuccessAttempt = guardedIdentity.login({"admin", "admin123"});
    assert(!lockedSuccessAttempt.success);
    assert(lockedSuccessAttempt.code == "AUTHENTICATION_LOCKED");

    auto productionPolicy = lockoutPolicy;
    productionPolicy.allowSeedCredentials = false;
    induspilot::modules::IdentityService productionIdentity(
        std::make_shared<induspilot::modules::InMemorySessionStore>(),
        std::chrono::hours(8),
        std::make_shared<induspilot::data::InMemoryUserRepository>(),
        std::make_shared<induspilot::data::InMemoryPermissionRepository>(),
        productionPolicy);
    const auto blockedSeedLogin = productionIdentity.login({"admin", "admin123"});
    assert(!blockedSeedLogin.success);
    assert(blockedSeedLogin.code == "SEED_CREDENTIAL_ROTATION_REQUIRED");

    assert(induspilot::modules::verifyPassword("admin123", "plain:admin123"));
    const auto generatedPasswordHash = induspilot::modules::generatePbkdf2Sha256PasswordHash("rotated-password", 100000);
    assert(generatedPasswordHash.rfind("pbkdf2_sha256$100000$", 0) == 0);
    assert(induspilot::modules::verifyPassword("rotated-password", generatedPasswordHash));
    assert(!induspilot::modules::verifyPassword("wrong-password", generatedPasswordHash));
    assert(induspilot::modules::verifyPassword("admin123", "pbkdf2_sha256$1000$identity-test-salt$3d8943413e05ec9118c53174a59bf506b84c558ead29bc37acd901f632a256f1"));
    assert(!induspilot::modules::verifyPassword("wrong-password", "pbkdf2_sha256$1000$identity-test-salt$3d8943413e05ec9118c53174a59bf506b84c558ead29bc37acd901f632a256f1"));
    assert(identity.logout(login.session->token));
    assert(!identity.validateSession(login.session->token).has_value());

    induspilot::modules::IdentityService expiringIdentity(std::make_shared<induspilot::modules::InMemorySessionStore>(), std::chrono::seconds(0));
    const auto expiringLogin = expiringIdentity.login({"operator", "operator123"});
    assert(expiringLogin.success);
    assert(expiringLogin.session.has_value());
    assert(!expiringIdentity.validateSession(expiringLogin.session->token).has_value());

    induspilot::data::InMemoryUserRepository users;
    assert(users.findByUsername("admin").has_value());
    const auto originalAdminHash = users.findByUsername("admin")->passwordHash;
    assert(users.findByUsername("admin")->requiresPasswordRotation);
    assert(users.updatePasswordHash("admin", generatedPasswordHash));
    assert(users.findByUsername("admin")->passwordHash == generatedPasswordHash);
    assert(users.findByUsername("admin")->credentialVersion == 2);
    assert(!users.findByUsername("admin")->requiresPasswordRotation);
    assert(originalAdminHash != generatedPasswordHash);
    assert(!users.updatePasswordHash("not-found", generatedPasswordHash));
    induspilot::data::InMemoryPermissionRepository permissionStore;
    assert(!permissionStore.permissionsForRoles({"admin"}).empty());
    induspilot::data::InMemoryAssetRepository assetStore;
    assert(assetStore.findById("asset-001").has_value());

    induspilot::modules::AssetService assets;
    assert(!assets.list().empty());
    assert(assets.updateLifecycleStatus("asset-001", induspilot::domain::AssetStatus::Maintenance));

    induspilot::modules::MonitoringService monitoring;
    const auto runtime = monitoring.updateState({"asset-001", "warning", "温度偏高", ""});
    assert(!runtime.updatedAt.empty());
    assert(monitoring.findState("asset-001").has_value());
    assert(!monitoring.listStates().empty());
    assert(monitoring.summarizeSeverity()["info"] == 1);
    assert(induspilot::modules::isSupportedRuntimeState("online"));
    assert(!induspilot::modules::isSupportedRuntimeState("invalid"));

    induspilot::modules::AlertService alerts;
    auto notificationRepository = std::make_shared<induspilot::data::InMemoryAlertRepository>();
    notificationRepository->saveNotification({
        "notice-lease", "alert-lease", "rule-lease", "console", "shift-lead", "queued", "lease test", 0, "", "", 0, 100, "lease-one", 3});
    assert(notificationRepository->claimDueNotifications(100, 200, 1, "lease-one").size() == 1);
    assert(notificationRepository->claimDueNotifications(150, 250, 1, "lease-two").empty());
    assert(notificationRepository->claimDueNotifications(201, 301, 1, "lease-two").size() == 1);
    alerts.createRule({"rule-invalid", "invalid delivery", "asset-001", "warning", "carrier-pigeon", "shift-lead", true});
    alerts.create({"alert-001", "asset-001", induspilot::domain::AlertSeverity::Critical, induspilot::domain::AlertState::Open, "温度异常", "", ""});
    const auto firstDispatch = alerts.dispatchQueuedNotifications();
    assert(firstDispatch.failed == 1);
    assert(firstDispatch.skipped == 0);
    const auto firstRetry = alerts.retryNotification("notice-alert-001-rule-invalid");
    assert(firstRetry.has_value());
    assert(firstRetry->status == "retrying");
    const auto secondRetry = alerts.retryNotification("notice-alert-001-rule-invalid");
    assert(secondRetry.has_value());
    assert(secondRetry->status == "dead_letter");
    assert(alerts.dispatchQueuedNotifications().sent == 0);
    induspilot::modules::AlertService externalAlerts;
    externalAlerts.createRule({"rule-webhook", "webhook delivery", "asset-001", "warning", "webhook", "http://127.0.0.1:1/notify", true});
    externalAlerts.create({"alert-webhook", "asset-001", induspilot::domain::AlertSeverity::Critical, induspilot::domain::AlertState::Open, "webhook test", "", ""});
    const auto webhookDispatch = externalAlerts.dispatchQueuedNotifications();
    assert(webhookDispatch.sent == 0);
    assert(externalAlerts.notifications().front().status == "retrying");
    auto blockedWebhookSender = induspilot::modules::makeAlertNotificationSender(
        induspilot::app::NotificationConfig{true, 1000, "example.com"});
    induspilot::modules::AlertService blockedWebhookAlerts(
        std::make_shared<induspilot::data::InMemoryAlertRepository>(), blockedWebhookSender);
    blockedWebhookAlerts.createRule({"rule-blocked-webhook", "blocked webhook", "asset-001", "warning", "webhook", "http://127.0.0.1:1/notify", true});
    blockedWebhookAlerts.create({"alert-blocked-webhook", "asset-001", induspilot::domain::AlertSeverity::Critical, induspilot::domain::AlertState::Open, "blocked webhook test", "", ""});
    blockedWebhookAlerts.dispatchQueuedNotifications();
    assert(blockedWebhookAlerts.notifications().front().lastError.find("不在允许列表") != std::string::npos);
#ifdef INDUSPILOT_WITH_DROGON
    auto privateWebhookSender = induspilot::modules::makeAlertNotificationSender(
        induspilot::app::NotificationConfig{true, 1000, "127.0.0.1"});
    induspilot::modules::AlertService privateWebhookAlerts(
        std::make_shared<induspilot::data::InMemoryAlertRepository>(), privateWebhookSender);
    privateWebhookAlerts.createRule({"rule-private-webhook", "private webhook", "asset-001", "warning", "webhook", "http://127.0.0.1:1/notify", true});
    privateWebhookAlerts.create({"alert-private-webhook", "asset-001", induspilot::domain::AlertSeverity::Critical, induspilot::domain::AlertState::Open, "private webhook test", "", ""});
    privateWebhookAlerts.dispatchQueuedNotifications();
    assert(privateWebhookAlerts.notifications().front().lastError.find("公网地址") != std::string::npos);
#endif
    assert(alerts.acknowledge("alert-001", "admin").has_value());
    assert(alerts.assign("alert-001", "maintainer").has_value());

    induspilot::modules::MaintenanceService maintenance;
    const auto alert = alerts.findById("alert-001").value();
    const auto order = maintenance.createFromAlert(alert, "检查主电机温度异常");
    assert(order.alertId == "alert-001");
    assert(maintenance.assign(order.id, "maintainer").has_value());
    assert(maintenance.startProcessing(order.id).has_value());
    assert(maintenance.complete(order.id, "已完成检查").has_value());
    assert(maintenance.close(order.id).has_value());
    assert(!maintenance.historyForAsset("asset-001").empty());

    induspilot::modules::AuditService audit;
    const auto auditEvent = audit.record(induspilot::domain::OperationAuditEvent{"audit-record-001", "admin", "test.record", "test", "test-001", "success", "trace-test", ""});
    assert(auditEvent.id == "audit-record-001");
    assert(!auditEvent.occurredAt.empty());
    assert(auditEvent.previousHash == "genesis");
    assert(!auditEvent.eventHash.empty());
    const auto secondAuditEvent = audit.record(induspilot::domain::OperationAuditEvent{"audit-record-002", "admin", "test.second", "test", "test-002", "success", "trace-test-2", ""});
    assert(secondAuditEvent.previousHash == auditEvent.eventHash);
    assert(!secondAuditEvent.eventHash.empty());
    const auto listedAuditEvents = audit.events();
    assert(listedAuditEvents.size() == 2);
    assert(listedAuditEvents.front().id == "audit-record-002");
    const auto auditIntegrity = audit.integrityReport();
    assert(auditIntegrity.verified);
    assert(auditIntegrity.total == 2);
    assert(auditIntegrity.latestHash == secondAuditEvent.eventHash);
    assert(!audit.events().empty());
    induspilot::modules::OperationAuditQuery auditQuery;
    auditQuery.actor = "admin";
    auditQuery.action = "test.record";
    assert(audit.events(auditQuery).size() == 1);
    auto retentionConfig = induspilot::app::AuditConfig{};
    retentionConfig.retentionDays = 1;
    auto retentionRepository = std::make_shared<induspilot::data::InMemoryOperationAuditRepository>();
    induspilot::modules::AuditService retainedAudit(retentionRepository, nullptr, retentionConfig);
    retainedAudit.record(induspilot::domain::OperationAuditEvent{"audit-retained-old", "admin", "test.old", "test", "old", "success", "trace-old", "2000-01-01T00:00:00"});
    retainedAudit.record(induspilot::domain::OperationAuditEvent{"audit-retained-new", "admin", "test.new", "test", "new", "success", "trace-new", ""});
    assert(retainedAudit.events().size() == 1);
    assert(retainedAudit.archiveEvents().size() == 2);
    assert(retainedAudit.integrityReport().verified);
    assert(retainedAudit.integrityReport().total == 2);
    const auto recordingSink = std::make_shared<RecordingAuditDeliverySink>();
    induspilot::modules::AuditService deliveredAudit(
        std::make_shared<induspilot::data::InMemoryOperationAuditRepository>(), recordingSink);
    deliveredAudit.record(induspilot::domain::OperationAuditEvent{"audit-delivery", "admin", "test.delivery", "test", "delivery", "success", "trace-delivery", ""});
    for (int attempt = 0; attempt < 50 && recordingSink->deliveryCount.load() == 0; ++attempt) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    assert(recordingSink->deliveryCount.load() == 1);
    auto retryConfig = induspilot::app::AuditConfig{};
    retryConfig.siemWebhookMaxAttempts = 2;
    retryConfig.siemWebhookPollMs = 10;
    const auto retryQueue = std::make_shared<induspilot::data::InMemoryAuditDeliveryQueueRepository>();
    const auto retrySink = std::make_shared<FailingAuditDeliverySink>();
    const auto retryMetrics = std::make_shared<induspilot::modules::MetricsRegistry>();
    {
        induspilot::modules::AuditService retryingAudit(
            std::make_shared<induspilot::data::InMemoryOperationAuditRepository>(),
            retrySink,
            retryConfig,
            retryQueue,
            retryMetrics);
        retryingAudit.record(induspilot::domain::OperationAuditEvent{
            "audit-siem-retry", "admin", "test.siem.retry", "test", "retry", "success", "trace-siem-retry", ""});
        for (int attempt = 0; attempt < 50 && retrySink->deliveryCount.load() == 0; ++attempt) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        assert(retrySink->deliveryCount.load() == 1);
        assert(retryQueue->depths().retrying == 1);
        assert(retryMetrics->renderPrometheus().find("induspilot_notification_deliveries_total{channel=\"siem\",outcome=\"retrying\"} 1") != std::string::npos);
        assert(retryMetrics->renderPrometheus().find("induspilot_audit_siem_delivery_queue{state=\"retrying\"} 1") != std::string::npos);
    }
    auto deadLetterConfig = retryConfig;
    deadLetterConfig.siemWebhookMaxAttempts = 1;
    const auto deadLetterQueue = std::make_shared<induspilot::data::InMemoryAuditDeliveryQueueRepository>();
    const auto deadLetterSink = std::make_shared<FailingAuditDeliverySink>();
    {
        induspilot::modules::AuditService deadLetterAudit(
            std::make_shared<induspilot::data::InMemoryOperationAuditRepository>(),
            deadLetterSink,
            deadLetterConfig,
            deadLetterQueue);
        deadLetterAudit.record(induspilot::domain::OperationAuditEvent{
            "audit-siem-dead-letter", "admin", "test.siem.dead", "test", "dead", "success", "trace-siem-dead", ""});
        for (int attempt = 0; attempt < 50 && deadLetterSink->deliveryCount.load() == 0; ++attempt) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        assert(deadLetterSink->deliveryCount.load() == 1);
        assert(deadLetterQueue->depths().deadLetter == 1);
    }
    auditQuery.occurredFrom = auditEvent.occurredAt;
    auditQuery.occurredTo = auditEvent.occurredAt;
    assert(audit.events(auditQuery).size() == 1);
    const auto duplicateAuditEvent = audit.record(induspilot::domain::OperationAuditEvent{
        "audit-record-001", "tampered-actor", "tampered.action", "tampered", "tampered-resource", "failed", "tampered-trace", ""});
    assert(duplicateAuditEvent.actor == auditEvent.actor);
    assert(duplicateAuditEvent.action == auditEvent.action);
    assert(duplicateAuditEvent.eventHash == auditEvent.eventHash);
    assert(audit.integrityReport().verified);
    induspilot::modules::MetricsRegistry metrics;
    metrics.recordHttpRequest("GET", "/health", 200, 2.5);
    metrics.recordHttpRequest("POST", "/api/v1/ai/diagnose", 200, 7.0);
    metrics.recordHttpRequest("POST", "/api/v1/alerts/alert-001/close", 200, 3.0);
    metrics.recordHttpRequest("POST", "/api/v1/work-orders/wo-001/close", 200, 4.0);
    metrics.recordHttpRequest("GET", "/api/v1/assets/not-exist", 404, 1.0);
    metrics.recordAiProviderCall("http", "diagnose", true, 12.5);
    metrics.recordAiProviderCall("provider-with-user-data", "prompt-secret", false, 2.0);
    metrics.recordAiInteractionOperation("read", true, 4.5);
    metrics.recordAiInteractionOperation("write", false, 1.5);
    metrics.recordAiInteractionOperation("operation-with-user-data", true, 3.0);
    metrics.recordNotificationDelivery("webhook", "retrying");
    metrics.recordNotificationDelivery("channel-with-user-data", "message-secret");
    metrics.recordReadiness({true, 4, 1, 2, 18, 1700000000000});
    assert(metrics.totalRequests() == 5);
    assert(metrics.totalErrors() == 1);
    assert(metrics.aiRequests() == 1);
    assert(metrics.alertClosures() == 1);
    assert(metrics.workOrderClosures() == 1);
    assert(induspilot::modules::normalizeMetricPath("/api/v1/assets/asset-001/status") == "/api/v1/assets/{id}/status");
    const auto metricsText = metrics.renderPrometheus();
    assert(metricsText.find("induspilot_http_requests_total 5") != std::string::npos);
    assert(metricsText.find("path=\"/api/v1/assets/{id}\"") != std::string::npos);
    assert(metricsText.find("induspilot_ai_requests_total 1") != std::string::npos);
    assert(metricsText.find("induspilot_ai_provider_calls_total{provider=\"http\",operation=\"diagnose\"} 1") != std::string::npos);
    assert(metricsText.find("prompt-secret") == std::string::npos);
    assert(metricsText.find("provider=\"unknown\"") != std::string::npos);
    assert(metricsText.find("induspilot_notification_deliveries_total{channel=\"webhook\",outcome=\"retrying\"} 1") != std::string::npos);
    assert(metricsText.find("message-secret") == std::string::npos);
    assert(metricsText.find("induspilot_readiness 1") != std::string::npos);
    assert(metricsText.find("induspilot_readiness_probes_total 4") != std::string::npos);
    assert(metricsText.find("induspilot_readiness_failures_total 1") != std::string::npos);
    assert(metricsText.find("induspilot_readiness_recoveries_total 2") != std::string::npos);
    assert(metricsText.find("induspilot_mongodb_ai_interaction_operations_total{operation=\"read\"} 1") != std::string::npos);
    assert(metricsText.find("induspilot_mongodb_ai_interaction_errors_total{operation=\"write\"} 1") != std::string::npos);
    assert(metricsText.find("operation-with-user-data") == std::string::npos);
    assert(metricsText.find("prompt-secret") == std::string::npos);
#ifdef INDUSPILOT_WITH_MONGODB
    std::string oversizedMongoDiagnostic(600, 'x');
    oversizedMongoDiagnostic += " mongodb://probe-user:probe-secret@127.0.0.1:27017/induspilot";
    oversizedMongoDiagnostic += " mongodb+srv://another-user:another-secret@cluster.example/induspilot";
    const auto sanitizedMongoFailure = induspilot::data::sanitizeMongoProbeFailure(oversizedMongoDiagnostic);
    assert(sanitizedMongoFailure.find("MongoDB authenticated ping failed") != std::string::npos);
    assert(sanitizedMongoFailure.find("probe-secret") == std::string::npos);
    assert(sanitizedMongoFailure.find("another-secret") == std::string::npos);
    assert(sanitizedMongoFailure.find("mongodb://") == std::string::npos);
    assert(sanitizedMongoFailure.find("mongodb+srv://") == std::string::npos);
    assert(sanitizedMongoFailure.size() <= 512);
    const auto mongoProbe = induspilot::data::probeMongoDb("mongodb://127.0.0.1:1", "", 25);
    assert(!mongoProbe.available);
    assert(mongoProbe.reason.find("database is empty") != std::string::npos);
#endif
    induspilot::modules::AiService ai;
    assert(ai.status().message.find("AI 未启用") != std::string::npos);
    induspilot::modules::AiService configuredAi(induspilot::app::AiConfig{true, "http", "http://127.0.0.1:9000"});
    assert(configuredAi.status().message.find("http://127.0.0.1:9000") != std::string::npos);
    const auto suggestion = ai.troubleshoot({"alert", "alert-001", "温度异常", {"设备：一号产线主电机"}});
    assert(!suggestion.available);
    const auto diagnosis = ai.diagnose(induspilot::modules::DiagnosisRequest{
        "alert",
        "alert-001",
        "一号产线主电机温度异常",
        induspilot::modules::DiagnosisContext{"asset-001", "温度异常", "warning", "critical", "温度偏高", "近期无关闭工单", "现场闻到异味", {"设备：一号产线主电机"}}});
    assert(diagnosis.riskLevel == "critical");
    assert(diagnosis.requiresHumanReview);
    assert(!diagnosis.possibleCauses.empty());
    assert(!diagnosis.recommendedActions.empty());
    assert(!ai.interactions().empty());

    return 0;
}
