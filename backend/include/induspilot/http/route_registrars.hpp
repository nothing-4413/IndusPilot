#pragma once

#include "induspilot/http/http_server_context.hpp"

#include <drogon/drogon.h>

namespace induspilot::http {

void registerAuthRoutes(drogon::HttpAppFramework& server, const HttpServerContext& context);
void registerAssetRoutes(drogon::HttpAppFramework& server, const HttpServerContext& context);
void registerMonitoringRoutes(drogon::HttpAppFramework& server, const HttpServerContext& context);
void registerAlertRoutes(drogon::HttpAppFramework& server, const HttpServerContext& context);
void registerWorkOrderRoutes(drogon::HttpAppFramework& server, const HttpServerContext& context);
void registerAuditRoutes(drogon::HttpAppFramework& server, const HttpServerContext& context);
void registerAiRoutes(drogon::HttpAppFramework& server, const HttpServerContext& context);
void registerPlatformRoutes(drogon::HttpAppFramework& server, const HttpServerContext& context);

}  // namespace induspilot::http
