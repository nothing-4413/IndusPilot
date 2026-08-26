#include "induspilot/http/route_registrars.hpp"

#include "induspilot/http/http_common.hpp"

namespace induspilot::http {
namespace {

Json::Value dependencyChecksToJson(const std::map<std::string, data::DependencyCheck>& dependencies) {
    Json::Value value;
    for (const auto& item : dependencies) {
        value[item.first]["required"] = item.second.required;
        value[item.first]["available"] = item.second.available;
        value[item.first]["reason"] = item.second.reason;
    }
    return value;
}

Json::Value startupToJson(const app::Application::StartupStatus& status) {
    Json::Value value;
    value["configurationValid"] = status.configurationValid;
    value["initialized"] = status.initialized;
    Json::Value errors(Json::arrayValue);
    for (const auto& error : status.errors) {
        errors.append(error);
    }
    value["errors"] = errors;
    return value;
}

}  // namespace

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

    server.registerHandler("/health/live", [application](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        writeRequestLog(request);
        Json::Value data;
        data["live"] = application->isRunning();
        callback(jsonResponse(responseEnvelope(application->isRunning(), application->isRunning() ? "OK" : "NOT_LIVE", "进程存活状态已生成", data),
                              application->isRunning() ? drogon::k200OK : drogon::k503ServiceUnavailable));
    }, {drogon::Get});

    server.registerHandler("/health/ready", [application](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        writeRequestLog(request);
        const auto status = application->readiness();
        Json::Value data;
        data["ready"] = status.ready;
        data["dependencies"] = dependencyChecksToJson(status.dependencies);
        callback(jsonResponse(responseEnvelope(status.ready, status.ready ? "OK" : "NOT_READY", "服务就绪状态已生成", data),
                              status.ready ? drogon::k200OK : drogon::k503ServiceUnavailable));
    }, {drogon::Get});

    server.registerHandler("/health/startup", [application](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        writeRequestLog(request);
        const auto status = application->startup();
        const auto successful = status.configurationValid && status.initialized;
        callback(jsonResponse(responseEnvelope(successful, successful ? "OK" : "STARTUP_FAILED", "服务启动状态已生成", startupToJson(status)),
                              successful ? drogon::k200OK : drogon::k503ServiceUnavailable));
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
