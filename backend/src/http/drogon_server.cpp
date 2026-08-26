#include "induspilot/http/drogon_server.hpp"

#ifdef INDUSPILOT_WITH_DROGON

#include "induspilot/app/application.hpp"
#include "induspilot/http/http_common.hpp"
#include "induspilot/http/http_server_context.hpp"
#include "induspilot/http/route_registrars.hpp"

#include <drogon/drogon.h>

namespace induspilot::http {
namespace {

void registerRoutes(const HttpServerContext& context) {
    registerTraceHeaders();
    auto& server = drogon::app();
    registerMetricsAdvice(context.metrics);
    registerPlatformRoutes(server, context);
    registerAuthRoutes(server, context);
    registerAssetRoutes(server, context);
    registerMonitoringRoutes(server, context);
    registerAlertRoutes(server, context);
    registerWorkOrderRoutes(server, context);
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
