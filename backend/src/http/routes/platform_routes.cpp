#include "induspilot/http/route_registrars.hpp"

#include "induspilot/http/http_common.hpp"

namespace induspilot::http {

void registerPlatformRoutes(drogon::HttpAppFramework& server, const HttpServerContext& context) {
    const auto& application = context.application;
    const auto& metrics = context.metrics;

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
}

}  // namespace induspilot::http
