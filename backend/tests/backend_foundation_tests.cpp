#include "induspilot/api/router.hpp"
#include "induspilot/app/application.hpp"
#include "induspilot/app/config.hpp"
#include "induspilot/data/repositories.hpp"
#include "induspilot/data/in_memory_repositories.hpp"
#ifdef INDUSPILOT_WITH_DROGON
#include "induspilot/data/mysql_repositories.hpp"
#endif
#include "induspilot/modules/ai_service.hpp"
#include "induspilot/modules/alert_service.hpp"
#include "induspilot/modules/audit_service.hpp"
#include "induspilot/modules/asset_service.hpp"
#include "induspilot/modules/identity_service.hpp"
#include "induspilot/modules/maintenance_service.hpp"
#include "induspilot/modules/monitoring_service.hpp"
#include "induspilot/modules/password_hasher.hpp"

#include <cassert>
#include <chrono>
#include <memory>
#include <string>
#include <type_traits>

static_assert(std::has_virtual_destructor_v<induspilot::data::UserRepository>);
static_assert(std::has_virtual_destructor_v<induspilot::data::AssetRepository>);
static_assert(std::has_virtual_destructor_v<induspilot::data::AlertRepository>);
static_assert(std::has_virtual_destructor_v<induspilot::data::WorkOrderRepository>);
static_assert(std::has_virtual_destructor_v<induspilot::data::AiInteractionRepository>);
static_assert(std::has_virtual_destructor_v<induspilot::data::OperationAuditRepository>);
#ifdef INDUSPILOT_WITH_DROGON
static_assert(std::is_base_of_v<induspilot::data::UserRepository, induspilot::data::MySqlUserRepository>);
static_assert(std::is_base_of_v<induspilot::data::AssetRepository, induspilot::data::MySqlAssetRepository>);
static_assert(std::is_base_of_v<induspilot::data::AlertRepository, induspilot::data::MySqlAlertRepository>);
static_assert(std::is_base_of_v<induspilot::data::WorkOrderRepository, induspilot::data::MySqlWorkOrderRepository>);
static_assert(std::is_base_of_v<induspilot::data::RuntimeStateRepository, induspilot::data::MySqlRuntimeStateRepository>);
static_assert(std::is_base_of_v<induspilot::data::AiInteractionRepository, induspilot::data::MySqlAiInteractionRepository>);
#endif

int main() {
#ifdef _WIN32
    _putenv_s("INDUSPILOT_SERVER_PORT", "18080");
    _putenv_s("INDUSPILOT_REDIS_SESSION_TTL_SECONDS", "60");
    _putenv_s("INDUSPILOT_REDIS_SESSION_KEY_PREFIX", "test:session:");
    _putenv_s("INDUSPILOT_REDIS_SESSION_STORE", "redis");
    _putenv_s("INDUSPILOT_REPOSITORY_STORE", "mysql");
    _putenv_s("INDUSPILOT_AI_PROVIDER", "http");
#else
    setenv("INDUSPILOT_SERVER_PORT", "18080", 1);
    setenv("INDUSPILOT_REDIS_SESSION_TTL_SECONDS", "60", 1);
    setenv("INDUSPILOT_REDIS_SESSION_KEY_PREFIX", "test:session:", 1);
    setenv("INDUSPILOT_REDIS_SESSION_STORE", "redis", 1);
    setenv("INDUSPILOT_REPOSITORY_STORE", "mysql", 1);
    setenv("INDUSPILOT_AI_PROVIDER", "http", 1);
#endif
    const auto loadedConfig = induspilot::app::loadConfig("config/backend.example.yaml");
    assert(loadedConfig.port == 18080);
    assert(loadedConfig.redis.sessionTtlSeconds == 60);
    assert(loadedConfig.redis.sessionKeyPrefix == "test:session:");
    assert(loadedConfig.redis.sessionStore == "redis");
    assert(loadedConfig.storage.repositoryStore == "mysql");
    assert(loadedConfig.ai.provider == "http");
#ifdef _WIN32
    _putenv_s("INDUSPILOT_SERVER_PORT", "");
    _putenv_s("INDUSPILOT_REDIS_SESSION_TTL_SECONDS", "");
    _putenv_s("INDUSPILOT_REDIS_SESSION_KEY_PREFIX", "");
    _putenv_s("INDUSPILOT_REDIS_SESSION_STORE", "");
    _putenv_s("INDUSPILOT_REPOSITORY_STORE", "");
    _putenv_s("INDUSPILOT_AI_PROVIDER", "");
#else
    unsetenv("INDUSPILOT_SERVER_PORT");
    unsetenv("INDUSPILOT_REDIS_SESSION_TTL_SECONDS");
    unsetenv("INDUSPILOT_REDIS_SESSION_KEY_PREFIX");
    unsetenv("INDUSPILOT_REDIS_SESSION_STORE");
    unsetenv("INDUSPILOT_REPOSITORY_STORE");
    unsetenv("INDUSPILOT_AI_PROVIDER");
#endif

    induspilot::app::Application app(induspilot::app::AppConfig{});
    app.start();
    assert(app.isRunning());
    assert(app.router().handle("GET", "/health").success);
    assert(!app.router().handle("GET", "/missing").success);

    induspilot::modules::IdentityService identity;
    const auto login = identity.login({"admin", "admin123"});
    assert(login.success);
    assert(login.session.has_value());
    assert(identity.validateSession(login.session->token).has_value());
    const auto permissions = identity.permissionsForRoles(login.session->user.roles);
    assert(identity.hasPermission(permissions, "asset:write"));
    assert(identity.hasPermission(permissions, "monitoring:write"));
    assert(identity.authenticate("admin", "admin123"));
    assert(!identity.authenticate("admin", "wrong-password"));
    assert(induspilot::modules::verifyPassword("admin123", "plain:admin123"));
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
    alerts.create({"alert-001", "asset-001", induspilot::domain::AlertSeverity::Critical, induspilot::domain::AlertState::Open, "温度异常", "", ""});
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
    const auto auditIntegrity = audit.integrityReport();
    assert(auditIntegrity.verified);
    assert(auditIntegrity.total == 2);
    assert(auditIntegrity.latestHash == secondAuditEvent.eventHash);
    assert(!audit.events().empty());
    induspilot::modules::OperationAuditQuery auditQuery;
    auditQuery.actor = "admin";
    auditQuery.action = "test.record";
    assert(audit.events(auditQuery).size() == 1);
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

    app.stop();
    assert(!app.isRunning());
    return 0;
}
