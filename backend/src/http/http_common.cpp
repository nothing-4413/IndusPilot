#include "induspilot/http/http_common.hpp"

#include "induspilot/domain/domain_types.hpp"

#include <atomic>
#include <chrono>
#include <exception>
#include <iostream>
#include <utility>

namespace induspilot::http {
namespace {

constexpr const char* kTraceIdAttribute = "induspilot.trace_id";
constexpr const char* kMetricsStartedAtAttribute = "induspilot.metrics_started_at";
constexpr const char* kRequestLeaseAttribute = "induspilot.request_lease";

bool isControlPlanePath(const std::string& path) {
    return path == "/health" || path == "/health/live" || path == "/health/ready" ||
        path == "/health/startup" || path == "/metrics";
}

std::string generatedTraceId() {
    static std::atomic<unsigned long long> sequence{0};
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    return "trace-" + std::to_string(millis) + '-' + std::to_string(++sequence);
}

std::string incomingTraceIdFor(const drogon::HttpRequestPtr& request) {
    const auto incomingTrace = request->getHeader("X-Trace-Id");
    if (!incomingTrace.empty()) {
        return incomingTrace;
    }
    const auto incomingRequest = request->getHeader("X-Request-Id");
    if (!incomingRequest.empty()) {
        return incomingRequest;
    }
    return {};
}

}  // namespace

Json::Value responseEnvelope(bool success, const std::string& code, const std::string& message, Json::Value data) {
    Json::Value value;
    value["success"] = success;
    value["code"] = code;
    value["message"] = message;
    value["data"] = std::move(data);
    return value;
}

drogon::HttpResponsePtr jsonResponse(const Json::Value& value, drogon::HttpStatusCode status) {
    auto response = drogon::HttpResponse::newHttpJsonResponse(value);
    response->setStatusCode(status);
    return response;
}

drogon::HttpResponsePtr invalidRequest(const std::string& message) {
    return jsonResponse(responseEnvelope(false, "INVALID_REQUEST", message), drogon::k400BadRequest);
}

drogon::HttpResponsePtr notFound(const std::string& message) {
    return jsonResponse(responseEnvelope(false, "RESOURCE_NOT_FOUND", message), drogon::k404NotFound);
}

drogon::HttpResponsePtr unauthorized() {
    return jsonResponse(
        responseEnvelope(false, "AUTHENTICATION_REQUIRED", "session is missing or expired"),
        drogon::k401Unauthorized);
}

drogon::HttpResponsePtr forbidden() {
    return jsonResponse(responseEnvelope(false, "AUTHORIZATION_DENIED", "permission denied"), drogon::k403Forbidden);
}

std::string traceIdFor(const drogon::HttpRequestPtr& request) {
    const auto storedTraceId = request->attributes()->get<std::string>(kTraceIdAttribute);
    if (!storedTraceId.empty()) {
        return storedTraceId;
    }
    auto traceId = incomingTraceIdFor(request);
    if (traceId.empty()) {
        traceId = generatedTraceId();
    }
    request->attributes()->insert(kTraceIdAttribute, traceId);
    return traceId;
}

void writeRequestLog(const drogon::HttpRequestPtr& request, const std::optional<modules::SessionInfo>& session) {
    std::cout << "{\"event\":\"http_request\"," <<
        "\"traceId\":\"" << traceIdFor(request) << "\"," <<
        "\"method\":\"" << request->methodString() << "\"," <<
        "\"path\":\"" << request->path() << "\"," <<
        "\"user\":\"" << (session ? session->user.username : "anonymous") << "\"}"
        << std::endl;
}

std::string bearerToken(const drogon::HttpRequestPtr& request) {
    const auto authorization = request->getHeader("Authorization");
    const std::string prefix = "Bearer ";
    if (authorization.rfind(prefix, 0) != 0) {
        return {};
    }
    return authorization.substr(prefix.size());
}

void recordAuditEvent(
    const std::shared_ptr<modules::AuditService>& audit,
    const std::string& actor,
    const std::string& action,
    const std::string& resourceType,
    const std::string& resourceId,
    const std::string& result,
    const std::string& traceId) {
    if (!audit) {
        return;
    }
    audit->record(domain::OperationAuditEvent{"", actor, action, resourceType, resourceId, result, traceId, ""});
}

std::optional<modules::SessionInfo> requireSession(
    const std::shared_ptr<modules::IdentityService>& identity,
    const drogon::HttpRequestPtr& request,
    const std::function<void(const drogon::HttpResponsePtr&)>& callback) {
    const auto session = identity->validateSession(bearerToken(request));
    if (!session) {
        callback(unauthorized());
        return std::nullopt;
    }
    return session;
}

bool requirePermission(
    const std::shared_ptr<modules::IdentityService>& identity,
    const modules::SessionInfo& session,
    const std::string& permission,
    const std::function<void(const drogon::HttpResponsePtr&)>& callback) {
    const auto permissions = identity->permissionsForRoles(session.user.roles);
    if (!identity->hasPermission(permissions, permission)) {
        callback(forbidden());
        return false;
    }
    return true;
}

bool parsePaginationParameter(
    const drogon::HttpRequestPtr& request,
    const std::string& name,
    int minValue,
    int maxValue,
    std::optional<int>& value,
    std::string& error) {
    const auto raw = request->getParameter(name);
    if (raw.empty()) {
        return true;
    }

    try {
        std::size_t consumed = 0;
        const auto parsed = std::stoi(raw, &consumed);
        if (consumed != raw.size() || parsed < minValue || parsed > maxValue) {
            error = name + " 必须是 " + std::to_string(minValue) + " 到 " + std::to_string(maxValue) + " 之间的整数";
            return false;
        }
        value = parsed;
        return true;
    } catch (const std::exception&) {
        error = name + " 必须是 " + std::to_string(minValue) + " 到 " + std::to_string(maxValue) + " 之间的整数";
        return false;
    }
}

void registerTraceHeaders() {
    static bool registered{false};
    if (registered) {
        return;
    }
    registered = true;
    drogon::app().registerPostHandlingAdvice([](const drogon::HttpRequestPtr& request, const drogon::HttpResponsePtr& response) {
        const auto traceId = traceIdFor(request);
        response->addHeader("X-Trace-Id", traceId);
        response->addHeader("X-Request-Id", traceId);
    });
}

void registerMetricsAdvice(const std::shared_ptr<modules::MetricsRegistry>& metrics) {
    static bool registered{false};
    if (registered) {
        return;
    }
    registered = true;
    drogon::app().registerPreHandlingAdvice([](const drogon::HttpRequestPtr& request) {
        request->attributes()->insert(kMetricsStartedAtAttribute, std::chrono::steady_clock::now());
    });
    drogon::app().registerPostHandlingAdvice([metrics](const drogon::HttpRequestPtr& request, const drogon::HttpResponsePtr& response) {
        const auto startedAt = request->attributes()->get<std::chrono::steady_clock::time_point>(kMetricsStartedAtAttribute);
        double durationMs = 0.0;
        if (startedAt != std::chrono::steady_clock::time_point{}) {
            const auto elapsed = std::chrono::steady_clock::now() - startedAt;
            durationMs = static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count()) / 1000.0;
        }
        metrics->recordHttpRequest(request->methodString(), request->path(), static_cast<int>(response->statusCode()), durationMs);
    });
}

void registerRequestLifecycleAdvice(drogon::HttpAppFramework& server, const HttpServerContext& context) {
    const auto& application = context.application;
    const auto& requestLifecycle = context.requestLifecycle;
    server.registerPreHandlingAdvice(
        [application, requestLifecycle](
            const drogon::HttpRequestPtr& request,
            drogon::AdviceCallback&& callback,
            drogon::AdviceChainCallback&& chain) {
            if (isControlPlanePath(request->path())) {
                chain();
                return;
            }

            if (!application->isRunning() || application->isDraining() || !requestLifecycle->tryBeginRequest()) {
                Json::Value data;
                data["draining"] = application->isDraining();
                callback(jsonResponse(
                    responseEnvelope(false, "SERVER_DRAINING", "服务正在停止，不再接受新请求", data),
                    drogon::k503ServiceUnavailable));
                return;
            }

            if (application->isDraining()) {
                requestLifecycle->finishRequest();
                Json::Value data;
                data["draining"] = true;
                callback(jsonResponse(
                    responseEnvelope(false, "SERVER_DRAINING", "服务正在停止，不再接受新请求", data),
                    drogon::k503ServiceUnavailable));
                return;
            }

            request->attributes()->insert(kRequestLeaseAttribute, true);
            chain();
        });
    server.registerPostHandlingAdvice(
        [requestLifecycle](const drogon::HttpRequestPtr& request, const drogon::HttpResponsePtr&) {
            if (!request->attributes()->get<bool>(kRequestLeaseAttribute)) {
                return;
            }
            request->attributes()->insert(kRequestLeaseAttribute, false);
            requestLifecycle->finishRequest();
        });
}

}  // namespace induspilot::http
