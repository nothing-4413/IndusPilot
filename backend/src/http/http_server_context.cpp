#include "induspilot/http/http_server_context.hpp"

#include "induspilot/data/in_memory_repositories.hpp"
#include "induspilot/data/mysql_repositories.hpp"

#ifdef INDUSPILOT_WITH_DROGON
#include <drogon/drogon.h>
#endif

#include <chrono>
#include <memory>
#include <stdexcept>
#include <utility>

namespace induspilot::http {
namespace {

std::shared_ptr<modules::SessionStore> createSessionStore(const app::AppConfig& config) {
#ifdef INDUSPILOT_WITH_REDIS
    if (config.redis.sessionStore == "redis") {
        return modules::makeRedisSessionStore(config.redis.uri, config.redis.sessionKeyPrefix);
    }
#endif
    return std::make_shared<modules::InMemorySessionStore>();
}

modules::LoginSecurityPolicy loginSecurityPolicyFrom(const app::AppConfig& config) {
    modules::LoginSecurityPolicy policy;
    policy.enabled = config.security.loginLockoutEnabled;
    policy.maxFailures = config.security.loginMaxFailures;
    policy.failureWindow = std::chrono::seconds(config.security.loginFailureWindowSeconds > 0 ? config.security.loginFailureWindowSeconds : 60);
    policy.lockDuration = std::chrono::seconds(config.security.loginLockoutSeconds > 0 ? config.security.loginLockoutSeconds : 900);
    return policy;
}

std::shared_ptr<modules::LoginRateLimiter> createLoginRateLimiter(const app::AppConfig& config) {
#ifdef INDUSPILOT_WITH_REDIS
    if (config.security.loginRateLimitStore == "redis") {
        return modules::makeRedisLoginRateLimiter(config.redis.uri);
    }
#else
    if (config.security.loginRateLimitStore == "redis") {
        throw std::runtime_error("Redis login rate limiting requires INDUSPILOT_WITH_REDIS");
    }
#endif
    return std::make_shared<modules::InMemoryLoginRateLimiter>();
}

modules::PasswordPolicy passwordPolicyFrom(const app::AppConfig& config) {
    return modules::PasswordPolicy{config.security.passwordMinLength, config.security.passwordIterations};
}

#ifdef INDUSPILOT_WITH_DROGON
std::shared_ptr<modules::IdentityService> createIdentityService(
    const app::AppConfig& config,
    const drogon::orm::DbClientPtr& mysqlClient) {
    const auto ttl = std::chrono::seconds(config.redis.sessionTtlSeconds > 0 ? config.redis.sessionTtlSeconds : 28800);
    const auto sessionStore = createSessionStore(config);
    const auto securityPolicy = loginSecurityPolicyFrom(config);
    const auto passwordPolicy = passwordPolicyFrom(config);
    const auto loginRateLimiter = createLoginRateLimiter(config);

    if (config.storage.repositoryStore == "mysql") {
        return std::make_shared<modules::IdentityService>(
            sessionStore,
            ttl,
            std::make_shared<data::MySqlUserRepository>(mysqlClient),
            std::make_shared<data::MySqlPermissionRepository>(mysqlClient),
            securityPolicy,
            passwordPolicy,
            loginRateLimiter);
    }

    return std::make_shared<modules::IdentityService>(
        sessionStore,
        ttl,
        std::make_shared<data::InMemoryUserRepository>(),
        std::make_shared<data::InMemoryPermissionRepository>(),
        securityPolicy,
        passwordPolicy,
        loginRateLimiter);
}

std::shared_ptr<data::AssetRepository> createAssetRepository(
    const app::AppConfig& config,
    const drogon::orm::DbClientPtr& mysqlClient) {
    if (config.storage.repositoryStore == "mysql") {
        return std::make_shared<data::MySqlAssetRepository>(mysqlClient);
    }
    return std::make_shared<data::InMemoryAssetRepository>();
}

std::shared_ptr<data::AlertRepository> createAlertRepository(
    const app::AppConfig& config,
    const drogon::orm::DbClientPtr& mysqlClient) {
    if (config.storage.repositoryStore == "mysql") {
        return std::make_shared<data::MySqlAlertRepository>(mysqlClient);
    }
    return std::make_shared<data::InMemoryAlertRepository>();
}

std::shared_ptr<data::WorkOrderRepository> createWorkOrderRepository(
    const app::AppConfig& config,
    const drogon::orm::DbClientPtr& mysqlClient) {
    if (config.storage.repositoryStore == "mysql") {
        return std::make_shared<data::MySqlWorkOrderRepository>(mysqlClient);
    }
    return std::make_shared<data::InMemoryWorkOrderRepository>();
}

std::shared_ptr<data::RuntimeStateRepository> createRuntimeStateRepository(
    const app::AppConfig& config,
    const drogon::orm::DbClientPtr& mysqlClient) {
    if (config.storage.repositoryStore == "mysql") {
        return std::make_shared<data::MySqlRuntimeStateRepository>(mysqlClient);
    }
    return std::make_shared<data::InMemoryRuntimeStateRepository>();
}

std::shared_ptr<data::OperationAuditRepository> createOperationAuditRepository(
    const app::AppConfig& config,
    const drogon::orm::DbClientPtr& mysqlClient) {
    if (config.storage.repositoryStore == "mysql") {
        return std::make_shared<data::MySqlOperationAuditRepository>(mysqlClient);
    }
    return std::make_shared<data::InMemoryOperationAuditRepository>();
}

std::shared_ptr<data::AiInteractionRepository> createAiInteractionRepository(
    const app::AppConfig& config,
    const drogon::orm::DbClientPtr& mysqlClient) {
    if (config.storage.repositoryStore == "mysql") {
        return std::make_shared<data::MySqlAiInteractionRepository>(mysqlClient);
    }
    return std::make_shared<data::InMemoryAiInteractionRepository>();
}
#endif

}  // namespace

HttpServerContext buildHttpServerContext(const app::AppConfig& config) {
    auto context = HttpServerContext{};
    context.application = std::make_shared<app::Application>(config);
    context.requestLifecycle = std::make_shared<HttpRequestLifecycle>();

#ifdef INDUSPILOT_WITH_DROGON
    drogon::orm::DbClientPtr mysqlClient;
    if (config.storage.repositoryStore == "mysql") {
        mysqlClient = data::makeMysqlClient(config.mysql, 2);
    }

    context.identity = createIdentityService(config, mysqlClient);
    context.assets = std::make_shared<modules::AssetService>(createAssetRepository(config, mysqlClient));
    context.monitoring = std::make_shared<modules::MonitoringService>(createRuntimeStateRepository(config, mysqlClient));
    context.alerts = std::make_shared<modules::AlertService>(createAlertRepository(config, mysqlClient));
    context.maintenance = std::make_shared<modules::MaintenanceService>(createWorkOrderRepository(config, mysqlClient));
    context.ai = std::make_shared<modules::AiService>(config.ai, createAiInteractionRepository(config, mysqlClient));
    context.audit = std::make_shared<modules::AuditService>(createOperationAuditRepository(config, mysqlClient));
    context.metrics = std::make_shared<modules::MetricsRegistry>();
#else
    context.identity = std::make_shared<modules::IdentityService>();
    context.assets = std::make_shared<modules::AssetService>();
    context.monitoring = std::make_shared<modules::MonitoringService>();
    context.alerts = std::make_shared<modules::AlertService>();
    context.maintenance = std::make_shared<modules::MaintenanceService>();
    context.ai = std::make_shared<modules::AiService>();
    context.audit = std::make_shared<modules::AuditService>();
    context.metrics = std::make_shared<modules::MetricsRegistry>();
#endif

    return context;
}

}  // namespace induspilot::http
