#include "induspilot/http/drogon_server.hpp"

#ifdef INDUSPILOT_WITH_DROGON

#include "induspilot/api/api_types.hpp"
#include "induspilot/app/application.hpp"
#include "induspilot/domain/domain_types.hpp"
#include "induspilot/modules/ai_service.hpp"
#include "induspilot/modules/alert_service.hpp"
#include "induspilot/modules/asset_service.hpp"
#include "induspilot/modules/identity_service.hpp"
#include "induspilot/modules/maintenance_service.hpp"
#include "induspilot/modules/monitoring_service.hpp"

#include <drogon/drogon.h>

#include <memory>
#include <string>
#include <vector>

namespace induspilot::http {
namespace {

std::string assetStatusToString(domain::AssetStatus status) {
    switch (status) {
        case domain::AssetStatus::Active:
            return "active";
        case domain::AssetStatus::Inactive:
            return "inactive";
        case domain::AssetStatus::Maintenance:
            return "maintenance";
        case domain::AssetStatus::Retired:
            return "retired";
    }
    return "unknown";
}


std::optional<domain::AssetStatus> tryAssetStatusFromString(const std::string& status) {
    if (status == "active") {
        return domain::AssetStatus::Active;
    }
    if (status == "inactive") {
        return domain::AssetStatus::Inactive;
    }
    if (status == "maintenance") {
        return domain::AssetStatus::Maintenance;
    }
    if (status == "retired") {
        return domain::AssetStatus::Retired;
    }
    return std::nullopt;
}

domain::AssetStatus assetStatusFromString(const std::string& status) {
    return tryAssetStatusFromString(status).value_or(domain::AssetStatus::Active);
}
std::string alertSeverityToString(domain::AlertSeverity severity) {
    switch (severity) {
        case domain::AlertSeverity::Info:
            return "info";
        case domain::AlertSeverity::Warning:
            return "warning";
        case domain::AlertSeverity::Critical:
            return "critical";
    }
    return "unknown";
}

std::string alertStateToString(domain::AlertState state) {
    switch (state) {
        case domain::AlertState::Open:
            return "open";
        case domain::AlertState::Acknowledged:
            return "acknowledged";
        case domain::AlertState::Assigned:
            return "assigned";
        case domain::AlertState::Resolved:
            return "resolved";
        case domain::AlertState::Closed:
            return "closed";
    }
    return "unknown";
}

std::string workOrderStateToString(domain::WorkOrderState state) {
    switch (state) {
        case domain::WorkOrderState::Created:
            return "created";
        case domain::WorkOrderState::Assigned:
            return "assigned";
        case domain::WorkOrderState::Processing:
            return "processing";
        case domain::WorkOrderState::Completed:
            return "completed";
        case domain::WorkOrderState::Closed:
            return "closed";
    }
    return "unknown";
}

Json::Value userToJson(const domain::User& user) {
    Json::Value value;
    value["id"] = user.id;
    value["username"] = user.username;
    for (const auto& role : user.roles) {
        value["roles"].append(role);
    }
    return value;
}

Json::Value sessionToJson(const modules::SessionInfo& session) {
    Json::Value value;
    value["token"] = session.token;
    value["active"] = session.active;
    value["user"] = userToJson(session.user);
    return value;
}

Json::Value assetToJson(const domain::EquipmentAsset& asset) {
    Json::Value value;
    value["id"] = asset.id;
    value["name"] = asset.name;
    value["type"] = asset.type;
    value["factory"] = asset.factory;
    value["workshop"] = asset.workshop;
    value["productionLine"] = asset.productionLine;
    value["status"] = assetStatusToString(asset.status);
    return value;
}

std::optional<modules::AssetQuery> assetQueryFromRequest(const drogon::HttpRequestPtr& request, std::string& error) {
    modules::AssetQuery query;
    const auto factory = request->getParameter("factory");
    const auto workshop = request->getParameter("workshop");
    const auto productionLine = request->getParameter("productionLine");
    const auto status = request->getParameter("status");

    if (!factory.empty()) {
        query.factory = factory;
    }
    if (!workshop.empty()) {
        query.workshop = workshop;
    }
    if (!productionLine.empty()) {
        query.productionLine = productionLine;
    }
    if (!status.empty()) {
        const auto parsed = tryAssetStatusFromString(status);
        if (!parsed) {
            error = "unsupported asset status";
            return std::nullopt;
        }
        query.status = *parsed;
    }
    return query;
}
Json::Value runtimeStateToJson(const modules::RuntimeState& state) {
    Json::Value value;
    value["assetId"] = state.assetId;
    value["state"] = state.state;
    value["metricSummary"] = state.metricSummary;
    value["updatedAt"] = state.updatedAt;
    value["severity"] = state.severity;
    return value;
}

Json::Value alertToJson(const domain::Alert& alert) {
    Json::Value value;
    value["id"] = alert.id;
    value["assetId"] = alert.assetId;
    value["severity"] = alertSeverityToString(alert.severity);
    value["state"] = alertStateToString(alert.state);
    value["title"] = alert.title;
    value["acknowledgedBy"] = alert.acknowledgedBy;
    value["assignedTo"] = alert.assignedTo;
    return value;
}

std::optional<modules::AlertQuery> alertQueryFromRequest(const drogon::HttpRequestPtr& request, std::string& error) {
    modules::AlertQuery query;
    const auto assetId = request->getParameter("assetId");
    const auto severity = request->getParameter("severity");
    const auto state = request->getParameter("state");

    if (!assetId.empty()) {
        query.assetId = assetId;
    }
    if (!severity.empty()) {
        const auto parsed = modules::alertSeverityFromString(severity);
        if (!parsed) {
            error = "unsupported alert severity";
            return std::nullopt;
        }
        query.severity = *parsed;
    }
    if (!state.empty()) {
        const auto parsed = modules::alertStateFromString(state);
        if (!parsed) {
            error = "unsupported alert state";
            return std::nullopt;
        }
        query.state = *parsed;
    }
    return query;
}
Json::Value workOrderToJson(const domain::WorkOrder& order) {
    Json::Value value;
    value["id"] = order.id;
    value["assetId"] = order.assetId;
    value["alertId"] = order.alertId;
    value["state"] = workOrderStateToString(order.state);
    value["assignee"] = order.assignee;
    value["summary"] = order.summary;
    value["result"] = order.result;
    return value;
}

std::optional<modules::WorkOrderQuery> workOrderQueryFromRequest(const drogon::HttpRequestPtr& request, std::string& error) {
    modules::WorkOrderQuery query;
    const auto assetId = request->getParameter("assetId");
    const auto alertId = request->getParameter("alertId");
    const auto state = request->getParameter("state");

    if (!assetId.empty()) {
        query.assetId = assetId;
    }
    if (!alertId.empty()) {
        query.alertId = alertId;
    }
    if (!state.empty()) {
        const auto parsed = modules::workOrderStateFromString(state);
        if (!parsed) {
            error = "unsupported work order state";
            return std::nullopt;
        }
        query.state = *parsed;
    }
    return query;
}
Json::Value responseEnvelope(bool success, const std::string& code, const std::string& message, Json::Value data = Json::Value(Json::objectValue)) {
    Json::Value value;
    value["success"] = success;
    value["code"] = code;
    value["message"] = message;
    value["data"] = data;
    return value;
}

drogon::HttpResponsePtr jsonResponse(const Json::Value& value, drogon::HttpStatusCode status = drogon::k200OK) {
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
    return jsonResponse(responseEnvelope(false, "AUTHENTICATION_REQUIRED", "session is missing or expired"), drogon::k401Unauthorized);
}

drogon::HttpResponsePtr forbidden() {
    return jsonResponse(responseEnvelope(false, "AUTHORIZATION_DENIED", "permission denied"), drogon::k403Forbidden);
}

std::string traceIdFor(const drogon::HttpRequestPtr& request) {
    const auto incoming = request->getHeader("X-Trace-Id");
    if (!incoming.empty()) {
        return incoming;
    }
    static std::atomic<unsigned long long> sequence{0};
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    return "trace-" + std::to_string(millis) + '-' + std::to_string(++sequence);
}

void writeRequestLog(const drogon::HttpRequestPtr& request, const std::optional<modules::SessionInfo>& session = std::nullopt) {
    std::cout << "{\"event\":\"http_request\","
              << "\"traceId\":\"" << traceIdFor(request) << "\","
              << "\"method\":\"" << request->methodString() << "\","
              << "\"path\":\"" << request->path() << "\","
              << "\"user\":\"" << (session ? session->user.username : "anonymous") << "\"}"
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


std::shared_ptr<modules::IdentityService> createIdentityService(const app::AppConfig& config) {
    auto ttl = std::chrono::seconds(config.redis.sessionTtlSeconds > 0 ? config.redis.sessionTtlSeconds : 28800);
#ifdef INDUSPILOT_WITH_REDIS
    if (config.redis.sessionStore == "redis") {
        return std::make_shared<modules::IdentityService>(
            modules::makeRedisSessionStore(config.redis.uri, config.redis.sessionKeyPrefix), ttl);
    }
#endif
    return std::make_shared<modules::IdentityService>(std::make_shared<modules::InMemorySessionStore>(), ttl);
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
void registerRoutes(
    const std::shared_ptr<app::Application>& application,
    const std::shared_ptr<modules::IdentityService>& identity,
    const std::shared_ptr<modules::AssetService>& assets,
    const std::shared_ptr<modules::MonitoringService>& monitoring,
    const std::shared_ptr<modules::AlertService>& alerts,
    const std::shared_ptr<modules::MaintenanceService>& maintenance,
    const std::shared_ptr<modules::AiService>& ai) {
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

    server.registerHandler("/api/v1/auth/login", [identity](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        writeRequestLog(request);
        const auto payload = request->getJsonObject();
        if (!payload || !payload->isMember("username") || !payload->isMember("password")) {
            callback(invalidRequest("username and password are required"));
            return;
        }

        const auto result = identity->login({(*payload)["username"].asString(), (*payload)["password"].asString()});
        if (!result.success || !result.session) {
            callback(jsonResponse(responseEnvelope(false, "AUTHENTICATION_FAILED", "invalid username or password"), drogon::k401Unauthorized));
            return;
        }

        writeRequestLog(request, result.session);
        callback(jsonResponse(responseEnvelope(true, "OK", "login succeeded", sessionToJson(*result.session))));
    }, {drogon::Post});

    server.registerHandler("/api/v1/auth/session", [identity](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        const auto session = identity->validateSession(bearerToken(request));
        if (!session) {
            callback(unauthorized());
            return;
        }
        writeRequestLog(request, session);
        callback(jsonResponse(responseEnvelope(true, "OK", "session is valid", sessionToJson(*session))));
    }, {drogon::Get});

    server.registerHandler("/api/v1/auth/logout", [identity](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        if (!identity->logout(bearerToken(request))) {
            callback(unauthorized());
            return;
        }
        writeRequestLog(request);
        callback(jsonResponse(responseEnvelope(true, "OK", "logout succeeded")));
    }, {drogon::Post});


    server.registerHandler("/api/v1/assets/{1}", [identity, assets](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback, const std::string& assetId) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "asset:read", callback)) {
            return;
        }
        const auto asset = assets->findById(assetId);
        if (!asset) {
            callback(notFound("asset not found"));
            return;
        }
        callback(jsonResponse(responseEnvelope(true, "OK", "asset returned", assetToJson(*asset))));
    }, {drogon::Get});
    server.registerHandler("/api/v1/assets", [identity, assets](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "asset:read", callback)) {
            return;
        }
        writeRequestLog(request, session);
        std::string queryError;
        const auto query = assetQueryFromRequest(request, queryError);
        if (!query) {
            callback(invalidRequest(queryError));
            return;
        }
        Json::Value rows(Json::arrayValue);
        for (const auto& asset : assets->list(*query)) {
            rows.append(assetToJson(asset));
        }
        callback(jsonResponse(responseEnvelope(true, "OK", "assets returned", rows)));
    }, {drogon::Get});


    server.registerHandler("/api/v1/assets", [identity, assets](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "asset:write", callback)) {
            return;
        }
        writeRequestLog(request, session);
        const auto payload = request->getJsonObject();
        if (!payload || !payload->isMember("id") || !payload->isMember("name")) {
            callback(invalidRequest("id and name are required"));
            return;
        }
        auto status = domain::AssetStatus::Active;
        if (payload->isMember("status")) {
            const auto parsed = tryAssetStatusFromString((*payload)["status"].asString());
            if (!parsed) {
                callback(invalidRequest("unsupported asset status"));
                return;
            }
            status = *parsed;
        }
        const auto asset = assets->create(domain::EquipmentAsset{
            (*payload)["id"].asString(),
            (*payload)["name"].asString(),
            payload->isMember("type") ? (*payload)["type"].asString() : "equipment",
            payload->isMember("factory") ? (*payload)["factory"].asString() : "default-factory",
            payload->isMember("workshop") ? (*payload)["workshop"].asString() : "default-workshop",
            payload->isMember("productionLine") ? (*payload)["productionLine"].asString() : "default-line",
            status});
        callback(jsonResponse(responseEnvelope(true, "OK", "asset created", assetToJson(asset))));
    }, {drogon::Post});

    server.registerHandler("/api/v1/assets/{1}/status", [identity, assets](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback, const std::string& assetId) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "asset:write", callback)) {
            return;
        }
        writeRequestLog(request, session);
        const auto payload = request->getJsonObject();
        if (!payload || !payload->isMember("status")) {
            callback(invalidRequest("status is required"));
            return;
        }
        const auto parsed = tryAssetStatusFromString((*payload)["status"].asString());
        if (!parsed) {
            callback(invalidRequest("unsupported asset status"));
            return;
        }
        if (!assets->updateLifecycleStatus(assetId, *parsed)) {
            callback(notFound("asset not found"));
            return;
        }
        const auto asset = assets->findById(assetId);
        callback(jsonResponse(responseEnvelope(true, "OK", "asset status updated", assetToJson(*asset))));
    }, {drogon::Patch});
    server.registerHandler("/api/v1/monitoring/states", [identity, monitoring](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "asset:read", callback)) {
            return;
        }
        writeRequestLog(request, session);
        Json::Value data;
        Json::Value stateSummary;
        for (const auto& item : monitoring->summarizeStates()) {
            stateSummary[item.first] = item.second;
        }
        Json::Value severitySummary;
        for (const auto& item : monitoring->summarizeSeverity()) {
            severitySummary[item.first] = item.second;
        }
        Json::Value rows(Json::arrayValue);
        for (const auto& state : monitoring->listStates()) {
            rows.append(runtimeStateToJson(state));
        }
        data["summary"]["states"] = stateSummary;
        data["summary"]["severity"] = severitySummary;
        data["items"] = rows;
        callback(jsonResponse(responseEnvelope(true, "OK", "monitoring states returned", data)));
    }, {drogon::Get});

    server.registerHandler("/api/v1/monitoring/states/{1}", [identity, monitoring](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback, const std::string& assetId) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "asset:read", callback)) {
            return;
        }
        writeRequestLog(request, session);
        const auto state = monitoring->findState(assetId);
        if (!state) {
            callback(notFound("runtime state not found"));
            return;
        }
        callback(jsonResponse(responseEnvelope(true, "OK", "runtime state returned", runtimeStateToJson(*state))));
    }, {drogon::Get});

    server.registerHandler("/api/v1/monitoring/states", [identity, monitoring](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "monitoring:write", callback)) {
            return;
        }
        writeRequestLog(request, session);
        const auto payload = request->getJsonObject();
        if (!payload || !payload->isMember("assetId") || !payload->isMember("state")) {
            callback(invalidRequest("assetId and state are required"));
            return;
        }
        const auto runtimeState = (*payload)["state"].asString();
        if (!modules::isSupportedRuntimeState(runtimeState)) {
            callback(invalidRequest("unsupported runtime state"));
            return;
        }
        auto severity = payload->isMember("severity") ? (*payload)["severity"].asString() : "info";
        if (!modules::isSupportedRuntimeSeverity(severity)) {
            callback(invalidRequest("unsupported runtime severity"));
            return;
        }
        const auto state = monitoring->updateState(modules::RuntimeState{
            (*payload)["assetId"].asString(),
            runtimeState,
            payload->isMember("metricSummary") ? (*payload)["metricSummary"].asString() : "",
            payload->isMember("updatedAt") ? (*payload)["updatedAt"].asString() : "",
            severity});
        callback(jsonResponse(responseEnvelope(true, "OK", "runtime state updated", runtimeStateToJson(state))));
    }, {drogon::Post});

    server.registerHandler("/api/v1/alerts", [identity, alerts](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "alert:read", callback)) {
            return;
        }
        writeRequestLog(request, session);
        std::string queryError;
        const auto query = alertQueryFromRequest(request, queryError);
        if (!query) {
            callback(invalidRequest(queryError));
            return;
        }
        Json::Value rows(Json::arrayValue);
        for (const auto& alert : alerts->list(*query)) {
            rows.append(alertToJson(alert));
        }
        callback(jsonResponse(responseEnvelope(true, "OK", "alerts returned", rows)));
    }, {drogon::Get});

    server.registerHandler("/api/v1/alerts/{1}", [identity, alerts](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback, const std::string& alertId) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "alert:read", callback)) {
            return;
        }
        writeRequestLog(request, session);
        const auto alert = alerts->findById(alertId);
        if (!alert) {
            callback(notFound("alert not found"));
            return;
        }
        callback(jsonResponse(responseEnvelope(true, "OK", "alert returned", alertToJson(*alert))));
    }, {drogon::Get});

    server.registerHandler("/api/v1/alerts", [identity, alerts](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "alert:write", callback)) {
            return;
        }
        writeRequestLog(request, session);
        const auto payload = request->getJsonObject();
        if (!payload || !payload->isMember("id") || !payload->isMember("assetId") || !payload->isMember("severity") || !payload->isMember("title")) {
            callback(invalidRequest("id, assetId, severity and title are required"));
            return;
        }
        const auto severity = modules::alertSeverityFromString((*payload)["severity"].asString());
        if (!severity) {
            callback(invalidRequest("unsupported alert severity"));
            return;
        }
        auto state = domain::AlertState::Open;
        if (payload->isMember("state")) {
            const auto parsed = modules::alertStateFromString((*payload)["state"].asString());
            if (!parsed) {
                callback(invalidRequest("unsupported alert state"));
                return;
            }
            state = *parsed;
        }
        const auto alert = alerts->create(domain::Alert{
            (*payload)["id"].asString(),
            (*payload)["assetId"].asString(),
            *severity,
            state,
            (*payload)["title"].asString(),
            "",
            payload->isMember("assignedTo") ? (*payload)["assignedTo"].asString() : ""});
        callback(jsonResponse(responseEnvelope(true, "OK", "alert created", alertToJson(alert))));
    }, {drogon::Post});

    server.registerHandler("/api/v1/alerts/{1}/acknowledge", [identity, alerts](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback, const std::string& alertId) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "alert:write", callback)) {
            return;
        }
        writeRequestLog(request, session);
        const auto alert = alerts->acknowledge(alertId, session->user.username);
        if (!alert) {
            callback(notFound("alert not found"));
            return;
        }
        callback(jsonResponse(responseEnvelope(true, "OK", "alert acknowledged", alertToJson(*alert))));
    }, {drogon::Post});

    server.registerHandler("/api/v1/alerts/{1}/assign", [identity, alerts](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback, const std::string& alertId) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "alert:write", callback)) {
            return;
        }
        writeRequestLog(request, session);
        const auto payload = request->getJsonObject();
        if (!payload || !payload->isMember("assignee")) {
            callback(invalidRequest("assignee is required"));
            return;
        }
        const auto alert = alerts->assign(alertId, (*payload)["assignee"].asString());
        if (!alert) {
            callback(notFound("alert not found"));
            return;
        }
        callback(jsonResponse(responseEnvelope(true, "OK", "alert assigned", alertToJson(*alert))));
    }, {drogon::Post});

    server.registerHandler("/api/v1/alerts/{1}/resolve", [identity, alerts](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback, const std::string& alertId) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "alert:write", callback)) {
            return;
        }
        writeRequestLog(request, session);
        const auto alert = alerts->resolve(alertId);
        if (!alert) {
            callback(notFound("alert not found"));
            return;
        }
        callback(jsonResponse(responseEnvelope(true, "OK", "alert resolved", alertToJson(*alert))));
    }, {drogon::Post});

    server.registerHandler("/api/v1/alerts/{1}/close", [identity, alerts](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback, const std::string& alertId) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "alert:write", callback)) {
            return;
        }
        writeRequestLog(request, session);
        const auto alert = alerts->close(alertId);
        if (!alert) {
            callback(notFound("alert not found"));
            return;
        }
        callback(jsonResponse(responseEnvelope(true, "OK", "alert closed", alertToJson(*alert))));
    }, {drogon::Post});
    server.registerHandler("/api/v1/work-orders", [identity, maintenance](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "work-order:read", callback)) {
            return;
        }
        writeRequestLog(request, session);
        std::string queryError;
        const auto query = workOrderQueryFromRequest(request, queryError);
        if (!query) {
            callback(invalidRequest(queryError));
            return;
        }
        Json::Value rows(Json::arrayValue);
        for (const auto& order : maintenance->list(*query)) {
            rows.append(workOrderToJson(order));
        }
        callback(jsonResponse(responseEnvelope(true, "OK", "work orders returned", rows)));
    }, {drogon::Get});

    server.registerHandler("/api/v1/work-orders/{1}", [identity, maintenance](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback, const std::string& orderId) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "work-order:read", callback)) {
            return;
        }
        writeRequestLog(request, session);
        const auto order = maintenance->findById(orderId);
        if (!order) {
            callback(notFound("work order not found"));
            return;
        }
        callback(jsonResponse(responseEnvelope(true, "OK", "work order returned", workOrderToJson(*order))));
    }, {drogon::Get});

    server.registerHandler("/api/v1/work-orders", [identity, maintenance](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "work-order:write", callback)) {
            return;
        }
        writeRequestLog(request, session);
        const auto payload = request->getJsonObject();
        if (!payload || !payload->isMember("id") || !payload->isMember("assetId") || !payload->isMember("summary")) {
            callback(invalidRequest("id, assetId and summary are required"));
            return;
        }
        auto state = domain::WorkOrderState::Created;
        if (payload->isMember("state")) {
            const auto parsed = modules::workOrderStateFromString((*payload)["state"].asString());
            if (!parsed) {
                callback(invalidRequest("unsupported work order state"));
                return;
            }
            state = *parsed;
        }
        const auto order = maintenance->create(domain::WorkOrder{
            (*payload)["id"].asString(),
            (*payload)["assetId"].asString(),
            payload->isMember("alertId") ? (*payload)["alertId"].asString() : "",
            state,
            payload->isMember("assignee") ? (*payload)["assignee"].asString() : "",
            (*payload)["summary"].asString(),
            payload->isMember("result") ? (*payload)["result"].asString() : ""});
        callback(jsonResponse(responseEnvelope(true, "OK", "work order created", workOrderToJson(order))));
    }, {drogon::Post});

    server.registerHandler("/api/v1/work-orders/from-alert", [identity, alerts, maintenance](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "work-order:write", callback)) {
            return;
        }
        writeRequestLog(request, session);
        const auto payload = request->getJsonObject();
        if (!payload || !payload->isMember("alertId") || !payload->isMember("summary")) {
            callback(invalidRequest("alertId and summary are required"));
            return;
        }
        const auto alert = alerts->findById((*payload)["alertId"].asString());
        if (!alert) {
            callback(notFound("alert not found"));
            return;
        }
        const auto order = maintenance->createFromAlert(*alert, (*payload)["summary"].asString());
        callback(jsonResponse(responseEnvelope(true, "OK", "work order created from alert", workOrderToJson(order))));
    }, {drogon::Post});

    server.registerHandler("/api/v1/work-orders/{1}/assign", [identity, maintenance](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback, const std::string& orderId) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "work-order:write", callback)) {
            return;
        }
        writeRequestLog(request, session);
        const auto payload = request->getJsonObject();
        if (!payload || !payload->isMember("assignee")) {
            callback(invalidRequest("assignee is required"));
            return;
        }
        const auto order = maintenance->assign(orderId, (*payload)["assignee"].asString());
        if (!order) {
            callback(notFound("work order not found"));
            return;
        }
        callback(jsonResponse(responseEnvelope(true, "OK", "work order assigned", workOrderToJson(*order))));
    }, {drogon::Post});

    server.registerHandler("/api/v1/work-orders/{1}/start", [identity, maintenance](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback, const std::string& orderId) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "work-order:write", callback)) {
            return;
        }
        writeRequestLog(request, session);
        const auto order = maintenance->startProcessing(orderId);
        if (!order) {
            callback(notFound("work order not found"));
            return;
        }
        callback(jsonResponse(responseEnvelope(true, "OK", "work order processing", workOrderToJson(*order))));
    }, {drogon::Post});

    server.registerHandler("/api/v1/work-orders/{1}/complete", [identity, maintenance](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback, const std::string& orderId) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "work-order:write", callback)) {
            return;
        }
        writeRequestLog(request, session);
        const auto payload = request->getJsonObject();
        if (!payload || !payload->isMember("result")) {
            callback(invalidRequest("result is required"));
            return;
        }
        const auto order = maintenance->complete(orderId, (*payload)["result"].asString());
        if (!order) {
            callback(notFound("work order not found"));
            return;
        }
        callback(jsonResponse(responseEnvelope(true, "OK", "work order completed", workOrderToJson(*order))));
    }, {drogon::Post});

    server.registerHandler("/api/v1/work-orders/{1}/close", [identity, maintenance](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback, const std::string& orderId) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "work-order:write", callback)) {
            return;
        }
        writeRequestLog(request, session);
        const auto order = maintenance->close(orderId);
        if (!order) {
            callback(notFound("work order not found"));
            return;
        }
        callback(jsonResponse(responseEnvelope(true, "OK", "work order closed", workOrderToJson(*order))));
    }, {drogon::Post});

    server.registerHandler("/api/v1/assets/{1}/maintenance-history", [identity, maintenance](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback, const std::string& assetId) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "work-order:read", callback)) {
            return;
        }
        writeRequestLog(request, session);
        Json::Value rows(Json::arrayValue);
        for (const auto& order : maintenance->historyForAsset(assetId)) {
            rows.append(workOrderToJson(order));
        }
        callback(jsonResponse(responseEnvelope(true, "OK", "maintenance history returned", rows)));
    }, {drogon::Get});
    server.registerHandler("/api/v1/ai/status", [identity, ai](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "ai:use", callback)) {
            return;
        }
        writeRequestLog(request, session);
        const auto status = ai->status();
        Json::Value data;
        data["module"] = status.name;
        data["available"] = status.ready;
        data["message"] = status.message;
        callback(jsonResponse(responseEnvelope(true, "OK", "AI status returned", data)));
    }, {drogon::Get});
}

}  // namespace

int runDrogonServer(const app::AppConfig& config) {
    auto application = std::make_shared<app::Application>(config);
    auto identity = createIdentityService(config);
    auto assets = std::make_shared<modules::AssetService>();
    auto monitoring = std::make_shared<modules::MonitoringService>();
    auto alerts = std::make_shared<modules::AlertService>();
    auto maintenance = std::make_shared<modules::MaintenanceService>();
    auto ai = std::make_shared<modules::AiService>();

    application->start();
    registerRoutes(application, identity, assets, monitoring, alerts, maintenance, ai);

    drogon::app().addListener(config.host, config.port).run();
    application->stop();
    return 0;
}

}  // namespace induspilot::http

#endif  // INDUSPILOT_WITH_DROGON
