#include "induspilot/api/router.hpp"
#include "induspilot/app/application.hpp"
#include "induspilot/app/config.hpp"
#include "induspilot/modules/ai_service.hpp"
#include "induspilot/modules/alert_service.hpp"
#include "induspilot/modules/asset_service.hpp"
#include "induspilot/modules/identity_service.hpp"
#include "induspilot/modules/maintenance_service.hpp"
#include "induspilot/modules/monitoring_service.hpp"

#include <cassert>

int main() {
    induspilot::app::Application app(induspilot::app::AppConfig{});
    app.start();
    assert(app.isRunning());
    assert(app.router().handle("GET", "/health").success);
    assert(!app.router().handle("GET", "/missing").success);

    induspilot::modules::IdentityService identity;
    const auto login = identity.login({"admin", "admin123"});
    assert(login.success);
    assert(login.session.has_value());
    const auto permissions = identity.permissionsForRoles(login.session->user.roles);
    assert(identity.hasPermission(permissions, "asset:write"));

    induspilot::modules::AssetService assets;
    assert(!assets.list().empty());
    assert(assets.updateLifecycleStatus("asset-001", induspilot::domain::AssetStatus::Maintenance));

    induspilot::modules::MonitoringService monitoring;
    monitoring.updateState({"asset-001", "warning", "温度偏高", "now"});
    assert(monitoring.findState("asset-001").has_value());

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