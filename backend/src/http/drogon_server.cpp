#include "induspilot/http/drogon_server.hpp"

#ifdef INDUSPILOT_WITH_DROGON

#include "induspilot/api/api_types.hpp"
#include "induspilot/app/application.hpp"
#include "induspilot/domain/domain_types.hpp"
#include "induspilot/http/http_common.hpp"
#include "induspilot/http/http_server_context.hpp"
#include "induspilot/http/route_registrars.hpp"
#include "induspilot/modules/audit_service.hpp"
#include "induspilot/modules/asset_service.hpp"
#include "induspilot/modules/identity_service.hpp"
#include "induspilot/modules/metrics_service.hpp"

#include <drogon/drogon.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <sstream>
#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace induspilot::http {
namespace {

void registerRoutes(const HttpServerContext& context) {
    const auto& application = context.application;
    const auto& identity = context.identity;
    const auto& ai = context.ai;
    const auto& audit = context.audit;
    const auto& metrics = context.metrics;
    registerTraceHeaders();
    registerMetricsAdvice(metrics);
    auto& server = drogon::app();

    server.registerHandler("/health", [application](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        writeRequestLog(request);
        const auto health = application->health();
        Json::Value dependencies;
        for (const auto& item : health.dependencies) {
            dependencies[item.first] = item.second;
        }

        Json::Value warnings(Json::arrayValue);
        for (const auto& warning : health.warnings) {
            warnings.append(warning);
        }

        Json::Value data;
        data["service"] = health.service;
        data["dependencies"] = dependencies;
        data["warnings"] = warnings;
        callback(jsonResponse(data));
    }, {drogon::Get});

    server.registerHandler("/metrics", [metrics](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        writeRequestLog(request);
        auto response = drogon::HttpResponse::newHttpResponse();
        response->setStatusCode(drogon::k200OK);
        response->setContentTypeCode(drogon::CT_TEXT_PLAIN);
        response->addHeader("Content-Type", "text/plain; version=0.0.4; charset=utf-8");
        response->setBody(metrics->renderPrometheus());
        callback(response);
    }, {drogon::Get});

    registerAuthRoutes(server, context);
    registerAssetRoutes(server, context);
    registerMonitoringRoutes(server, context);
    registerAlertRoutes(server, context);
    registerWorkOrderRoutes(server, context);
    registerAuditRoutes(server, context);
    registerAiRoutes(server, context);

    registerAuditRoutes(server, context);
    registerAiRoutes(server, context);
}

}  // namespace

int runDrogonServer(const app::AppConfig& config) {
    const auto context = buildHttpServerContext(config);

    context.application->start();
    registerRoutes(context);

    drogon::app().addListener(config.host, config.port).run();
    context.application->stop();
    return 0;
}

}  // namespace induspilot::http

#endif  // INDUSPILOT_WITH_DROGON
