#pragma once

#include "induspilot/http/http_server_context.hpp"

#include <drogon/drogon.h>

#include <functional>
#include <optional>
#include <string>

namespace induspilot::http {

Json::Value responseEnvelope(
    bool success,
    const std::string& code,
    const std::string& message,
    Json::Value data = Json::Value(Json::objectValue));
drogon::HttpResponsePtr jsonResponse(
    const Json::Value& value,
    drogon::HttpStatusCode status = drogon::k200OK);
drogon::HttpResponsePtr invalidRequest(const std::string& message);
drogon::HttpResponsePtr notFound(const std::string& message);
drogon::HttpResponsePtr unauthorized();
drogon::HttpResponsePtr forbidden();
drogon::HttpResponsePtr dependencyUnavailable(const std::string& message);

std::string traceIdFor(const drogon::HttpRequestPtr& request);
std::string bearerToken(const drogon::HttpRequestPtr& request);
void writeRequestLog(
    const drogon::HttpRequestPtr& request,
    const std::optional<modules::SessionInfo>& session = std::nullopt);
void recordAuditEvent(
    const std::shared_ptr<modules::AuditService>& audit,
    const std::string& actor,
    const std::string& action,
    const std::string& resourceType,
    const std::string& resourceId,
    const std::string& result,
    const std::string& traceId);
std::optional<modules::SessionInfo> requireSession(
    const std::shared_ptr<modules::IdentityService>& identity,
    const drogon::HttpRequestPtr& request,
    const std::function<void(const drogon::HttpResponsePtr&)>& callback);
bool requirePermission(
    const std::shared_ptr<modules::IdentityService>& identity,
    const modules::SessionInfo& session,
    const std::string& permission,
    const std::function<void(const drogon::HttpResponsePtr&)>& callback);

bool parsePaginationParameter(
    const drogon::HttpRequestPtr& request,
    const std::string& name,
    int minValue,
    int maxValue,
    std::optional<int>& value,
    std::string& error);

void registerTraceHeaders();
void registerMetricsAdvice(const std::shared_ptr<modules::MetricsRegistry>& metrics);
void registerRequestLifecycleAdvice(
    drogon::HttpAppFramework& server,
    const HttpServerContext& context);

}  // namespace induspilot::http
