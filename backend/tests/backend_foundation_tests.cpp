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
#include "induspilot/modules/asset_service.hpp"
#include "induspilot/modules/identity_service.hpp"
#include "induspilot/modules/maintenance_service.hpp"
#include "induspilot/modules/monitoring_service.hpp"

#include <cassert>
#include <chrono>
#include <memory>
#include <type_traits>

static_assert(std::has_virtual_destructor_v<induspilot::data::UserRepository>);
static_assert(std::has_virtual_destructor_v<induspilot::data::AssetRepository>);
static_assert(std::has_virtual_destructor_v<induspilot::data::AlertRepository>);
static_assert(std::has_virtual_destructor_v<induspilot::data::WorkOrderRepository>);
static_assert(std::has_virtual_destructor_v<induspilot::data::AiInteractionRepository>);
#ifdef INDUSPILOT_WITH_DROGON
static_assert(std::is_base_of_v<induspilot::data::UserRepository, induspilot::data::MySqlUserRepository>);
static_assert(std::is_base_of_v<induspilot::data::AssetRepository, induspilot::data::MySqlAssetRepository>);
#endif

int main() {
#ifdef _WIN32
    _putenv_s("INDUSPILOT_SERVER_PORT", "18080");
    _putenv_s("INDUSPILOT_REDIS_SESSION_TTL_SECONDS", "60");
    _putenv_s("INDUSPILOT_REDIS_SESSION_KEY_PREFIX", "test:session:");
    _putenv_s("INDUSPILOT_REDIS_SESSION_STORE", "redis");
#else
    setenv("INDUSPILOT_SERVER_PORT", "18080", 1);
    setenv("INDUSPILOT_REDIS_SESSION_TTL_SECONDS", "60", 1);
    setenv("INDUSPILOT_REDIS_SESSION_KEY_PREFIX", "test:session:", 1);
    setenv("INDUSPILOT_REDIS_SESSION_STORE", "redis", 1);
#endif
    const auto loadedConfig = induspilot::app::loadConfig("config/backend.example.yaml");
    assert(loadedConfig.port == 18080);
    assert(loadedConfig.redis.sessionTtlSeconds == 60);
    assert(loadedConfig.redis.sessionKeyPrefix == "test:session:");
    assert(loadedConfig.redis.sessionStore == "redis");
#ifdef _WIN32
    _putenv_s("INDUSPILOT_SERVER_PORT", "");
    _putenv_s("INDUSPILOT_REDIS_SESSION_TTL_SECONDS", "");
    _putenv_s("INDUSPILOT_REDIS_SESSION_KEY_PREFIX", "");
    _putenv_s("INDUSPILOT_REDIS_SESSION_STORE", "");
#else
    unsetenv("INDUSPILOT_SERVER_PORT");
    unsetenv("INDUSPILOT_REDIS_SESSION_TTL_SECONDS");
    unsetenv("INDUSPILOT_REDIS_SESSION_KEY_PREFIX");
    unsetenv("INDUSPILOT_REDIS_SESSION_STORE");
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

    induspilot::modules::AiService ai;
    const auto suggestion = ai.troubleshoot({"alert", "alert-001", "温度异常", {"设备：一号产线主电机"}});
    assert(!suggestion.available);
    assert(!ai.interactions().empty());

    app.stop();
    assert(!app.isRunning());
    return 0;
}
