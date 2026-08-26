#pragma once

#include "induspilot/app/application.hpp"
#include "induspilot/modules/ai_service.hpp"
#include "induspilot/modules/alert_service.hpp"
#include "induspilot/modules/audit_service.hpp"
#include "induspilot/modules/asset_service.hpp"
#include "induspilot/modules/identity_service.hpp"
#include "induspilot/modules/maintenance_service.hpp"
#include "induspilot/modules/metrics_service.hpp"
#include "induspilot/modules/monitoring_service.hpp"

#include <memory>

namespace induspilot::http {

struct HttpServerContext {
    std::shared_ptr<app::Application> application;
    std::shared_ptr<modules::IdentityService> identity;
    std::shared_ptr<modules::AssetService> assets;
    std::shared_ptr<modules::MonitoringService> monitoring;
    std::shared_ptr<modules::AlertService> alerts;
    std::shared_ptr<modules::MaintenanceService> maintenance;
    std::shared_ptr<modules::AiService> ai;
    std::shared_ptr<modules::AuditService> audit;
    std::shared_ptr<modules::MetricsRegistry> metrics;
};

HttpServerContext buildHttpServerContext(const app::AppConfig& config);

}  // namespace induspilot::http
