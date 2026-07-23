#include "induspilot/http/drogon_server.hpp"

#ifdef INDUSPILOT_WITH_DROGON

#include "induspilot/api/api_types.hpp"
#include "induspilot/app/application.hpp"
#include "induspilot/data/in_memory_repositories.hpp"
#include "induspilot/data/mysql_repositories.hpp"
#include "induspilot/domain/domain_types.hpp"
#include "induspilot/modules/ai_service.hpp"
#include "induspilot/modules/alert_service.hpp"
#include "induspilot/modules/asset_service.hpp"
#include "induspilot/modules/identity_service.hpp"
#include "induspilot/modules/maintenance_service.hpp"
#include "induspilot/modules/monitoring_service.hpp"

#include <drogon/drogon.h>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
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

Json::Value workOrderAttachmentToJson(const domain::WorkOrderAttachment& attachment) {
    Json::Value value;
    value["id"] = attachment.id;
    value["workOrderId"] = attachment.workOrderId;
    value["fileName"] = attachment.fileName;
    value["uri"] = attachment.uri;
    value["contentType"] = attachment.contentType;
    value["sizeBytes"] = static_cast<Json::UInt64>(attachment.sizeBytes);
    value["uploadedBy"] = attachment.uploadedBy;
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

Json::Value aiSuggestionToJson(const modules::AiSuggestion& suggestion) {
    Json::Value value;
    value["available"] = suggestion.available;
    value["label"] = suggestion.label;
    value["content"] = suggestion.content;
    return value;
}

Json::Value aiInteractionToJson(const domain::AiInteraction& interaction) {
    Json::Value value;
    value["id"] = interaction.id;
    value["relatedType"] = interaction.relatedType;
    value["relatedId"] = interaction.relatedId;
    value["input"] = interaction.input;
    value["output"] = interaction.output;
    return value;
}

std::optional<modules::AiRequest> aiRequestFromPayload(const Json::Value& payload, std::string& error) {
    if (!payload.isMember("relatedType") || !payload.isMember("relatedId") || !payload.isMember("prompt")) {
        error = "relatedType, relatedId and prompt are required";
        return std::nullopt;
    }
    modules::AiRequest request;
    request.relatedType = payload["relatedType"].asString();
    request.relatedId = payload["relatedId"].asString();
    request.prompt = payload["prompt"].asString();
    if (payload.isMember("contextItems") && payload["contextItems"].isArray()) {
        for (const auto& item : payload["contextItems"]) {
            request.contextItems.push_back(item.asString());
        }
    }
    return request;
}
Json::Value stringArrayToJson(const std::vector<std::string>& items) {
    Json::Value value(Json::arrayValue);
    for (const auto& item : items) {
        value.append(item);
    }
    return value;
}

Json::Value diagnosisResultToJson(const modules::DiagnosisResult& result) {
    Json::Value value;
    value["available"] = result.available;
    value["provider"] = result.provider;
    value["summary"] = result.summary;
    value["possibleCauses"] = stringArrayToJson(result.possibleCauses);
    value["recommendedActions"] = stringArrayToJson(result.recommendedActions);
    value["riskLevel"] = result.riskLevel;
    value["requiresHumanReview"] = result.requiresHumanReview;
    value["rawProviderOutput"] = result.rawProviderOutput;
    return value;
}

std::optional<modules::DiagnosisRequest> diagnosisRequestFromPayload(const Json::Value& payload, std::string& error) {
    if (!payload.isMember("relatedType") || !payload.isMember("relatedId") || !payload.isMember("prompt")) {
        error = "relatedType, relatedId and prompt are required";
        return std::nullopt;
    }

    modules::DiagnosisRequest request;
    request.relatedType = payload["relatedType"].asString();
    request.relatedId = payload["relatedId"].asString();
    request.prompt = payload["prompt"].asString();

    if (payload.isMember("context") && payload["context"].isObject()) {
        const auto& context = payload["context"];
        request.context.assetId = context.isMember("assetId") ? context["assetId"].asString() : "";
        request.context.alertTitle = context.isMember("alertTitle") ? context["alertTitle"].asString() : "";
        request.context.runtimeState = context.isMember("runtimeState") ? context["runtimeState"].asString() : "";
        request.context.severity = context.isMember("severity") ? context["severity"].asString() : "";
        request.context.metricSummary = context.isMember("metricSummary") ? context["metricSummary"].asString() : "";
        request.context.workOrderHistory = context.isMember("workOrderHistory") ? context["workOrderHistory"].asString() : "";
        request.context.operatorDescription = context.isMember("operatorDescription") ? context["operatorDescription"].asString() : "";
        if (context.isMember("contextItems") && context["contextItems"].isArray()) {
            for (const auto& item : context["contextItems"]) {
                request.context.contextItems.push_back(item.asString());
            }
        }
    }

    if (payload.isMember("contextItems") && payload["contextItems"].isArray()) {
        for (const auto& item : payload["contextItems"]) {
            request.context.contextItems.push_back(item.asString());
        }
    }
    return request;
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


std::shared_ptr<modules::SessionStore> createSessionStore(const app::AppConfig& config) {
#ifdef INDUSPILOT_WITH_REDIS
    if (config.redis.sessionStore == "redis") {
        return modules::makeRedisSessionStore(config.redis.uri, config.redis.sessionKeyPrefix);
    }
#endif
    return std::make_shared<modules::InMemorySessionStore>();
}

std::shared_ptr<modules::IdentityService> createIdentityService(const app::AppConfig& config, const drogon::orm::DbClientPtr& mysqlClient) {
    auto ttl = std::chrono::seconds(config.redis.sessionTtlSeconds > 0 ? config.redis.sessionTtlSeconds : 28800);
    auto sessionStore = createSessionStore(config);

    if (config.storage.repositoryStore == "mysql") {
        return std::make_shared<modules::IdentityService>(
            sessionStore,
            ttl,
            std::make_shared<data::MySqlUserRepository>(mysqlClient),
            std::make_shared<data::MySqlPermissionRepository>(mysqlClient));
    }

    return std::make_shared<modules::IdentityService>(
        sessionStore,
        ttl,
        std::make_shared<data::InMemoryUserRepository>(),
        std::make_shared<data::InMemoryPermissionRepository>());
}

std::shared_ptr<data::AssetRepository> createAssetRepository(const app::AppConfig& config, const drogon::orm::DbClientPtr& mysqlClient) {
    if (config.storage.repositoryStore == "mysql") {
        return std::make_shared<data::MySqlAssetRepository>(mysqlClient);
    }
    return std::make_shared<data::InMemoryAssetRepository>();
}

std::shared_ptr<data::AlertRepository> createAlertRepository(const app::AppConfig& config, const drogon::orm::DbClientPtr& mysqlClient) {
    if (config.storage.repositoryStore == "mysql") {
        return std::make_shared<data::MySqlAlertRepository>(mysqlClient);
    }
    return std::make_shared<data::InMemoryAlertRepository>();
}

std::shared_ptr<data::WorkOrderRepository> createWorkOrderRepository(const app::AppConfig& config, const drogon::orm::DbClientPtr& mysqlClient) {
    if (config.storage.repositoryStore == "mysql") {
        return std::make_shared<data::MySqlWorkOrderRepository>(mysqlClient);
    }
    return std::make_shared<data::InMemoryWorkOrderRepository>();
}

std::shared_ptr<data::RuntimeStateRepository> createRuntimeStateRepository(const app::AppConfig& config, const drogon::orm::DbClientPtr& mysqlClient) {
    if (config.storage.repositoryStore == "mysql") {
        return std::make_shared<data::MySqlRuntimeStateRepository>(mysqlClient);
    }
    return std::make_shared<data::InMemoryRuntimeStateRepository>();
}

std::shared_ptr<data::AiInteractionRepository> createAiInteractionRepository(const app::AppConfig& config, const drogon::orm::DbClientPtr& mysqlClient) {
    if (config.storage.repositoryStore == "mysql") {
        return std::make_shared<data::MySqlAiInteractionRepository>(mysqlClient);
    }
    return std::make_shared<data::InMemoryAiInteractionRepository>();
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

    server.registerHandler("/api/v1/work-orders/{1}", [identity, maintenance](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback, const std::string& orderId) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "work-order:write", callback)) {
            return;
        }
        writeRequestLog(request, session);
        const auto payload = request->getJsonObject();
        if (!payload || (!payload->isMember("summary") && !payload->isMember("assignee") && !payload->isMember("result"))) {
            callback(invalidRequest("summary, assignee or result is required"));
            return;
        }
        modules::WorkOrderUpdate update;
        if (payload->isMember("summary")) {
            update.summary = (*payload)["summary"].asString();
        }
        if (payload->isMember("assignee")) {
            update.assignee = (*payload)["assignee"].asString();
        }
        if (payload->isMember("result")) {
            update.result = (*payload)["result"].asString();
        }
        const auto order = maintenance->update(orderId, update);
        if (!order) {
            callback(notFound("work order not found"));
            return;
        }
        callback(jsonResponse(responseEnvelope(true, "OK", "work order updated", workOrderToJson(*order))));
    }, {drogon::Patch});

    server.registerHandler("/api/v1/work-orders/{1}/attachments", [identity, maintenance](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback, const std::string& orderId) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "work-order:read", callback)) {
            return;
        }
        writeRequestLog(request, session);
        if (!maintenance->findById(orderId)) {
            callback(notFound("work order not found"));
            return;
        }
        Json::Value rows(Json::arrayValue);
        for (const auto& attachment : maintenance->attachmentsFor(orderId)) {
            rows.append(workOrderAttachmentToJson(attachment));
        }
        callback(jsonResponse(responseEnvelope(true, "OK", "work order attachments returned", rows)));
    }, {drogon::Get});

    server.registerHandler("/api/v1/work-orders/{1}/attachments", [identity, maintenance](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback, const std::string& orderId) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "work-order:write", callback)) {
            return;
        }
        writeRequestLog(request, session);
        const auto payload = request->getJsonObject();
        if (!payload || !payload->isMember("id") || !payload->isMember("fileName") || !payload->isMember("uri")) {
            callback(invalidRequest("id, fileName and uri are required"));
            return;
        }
        domain::WorkOrderAttachment attachment;
        attachment.id = (*payload)["id"].asString();
        attachment.fileName = (*payload)["fileName"].asString();
        attachment.uri = (*payload)["uri"].asString();
        attachment.contentType = payload->isMember("contentType") ? (*payload)["contentType"].asString() : "application/octet-stream";
        attachment.sizeBytes = payload->isMember("sizeBytes") ? (*payload)["sizeBytes"].asUInt64() : 0;
        attachment.uploadedBy = payload->isMember("uploadedBy") ? (*payload)["uploadedBy"].asString() : session->user.username;
        const auto saved = maintenance->addAttachment(orderId, attachment);
        if (!saved) {
            callback(notFound("work order not found"));
            return;
        }
        callback(jsonResponse(responseEnvelope(true, "OK", "work order attachment registered", workOrderAttachmentToJson(*saved))));
    }, {drogon::Post});
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
        data["provider"] = ai->providerName();
        data["endpoint"] = ai->providerEndpoint();
        callback(jsonResponse(responseEnvelope(true, "OK", "AI status returned", data)));
    }, {drogon::Get});

    server.registerHandler("/api/v1/ai/troubleshoot", [identity, ai](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "ai:use", callback)) {
            return;
        }
        writeRequestLog(request, session);
        const auto payload = request->getJsonObject();
        if (!payload) {
            callback(invalidRequest("JSON body is required"));
            return;
        }
        std::string error;
        const auto aiRequest = aiRequestFromPayload(*payload, error);
        if (!aiRequest) {
            callback(invalidRequest(error));
            return;
        }
        const auto suggestion = ai->troubleshoot(*aiRequest);
        callback(jsonResponse(responseEnvelope(true, "OK", "AI troubleshooting returned", aiSuggestionToJson(suggestion))));
    }, {drogon::Post});

    server.registerHandler("/api/v1/ai/summarize-logs", [identity, ai](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "ai:use", callback)) {
            return;
        }
        writeRequestLog(request, session);
        const auto payload = request->getJsonObject();
        if (!payload) {
            callback(invalidRequest("JSON body is required"));
            return;
        }
        std::string error;
        const auto aiRequest = aiRequestFromPayload(*payload, error);
        if (!aiRequest) {
            callback(invalidRequest(error));
            return;
        }
        const auto suggestion = ai->summarizeLogs(*aiRequest);
        callback(jsonResponse(responseEnvelope(true, "OK", "AI log summary returned", aiSuggestionToJson(suggestion))));
    }, {drogon::Post});

    server.registerHandler("/api/v1/ai/diagnose", [identity, ai](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "ai:use", callback)) {
            return;
        }
        writeRequestLog(request, session);
        const auto payload = request->getJsonObject();
        if (!payload) {
            callback(invalidRequest("JSON body is required"));
            return;
        }
        std::string error;
        const auto diagnosisRequest = diagnosisRequestFromPayload(*payload, error);
        if (!diagnosisRequest) {
            callback(invalidRequest(error));
            return;
        }
        const auto diagnosis = ai->diagnose(*diagnosisRequest);
        callback(jsonResponse(responseEnvelope(true, "OK", "AI diagnosis returned", diagnosisResultToJson(diagnosis))));
    }, {drogon::Post});
    server.registerHandler("/api/v1/ai/interactions", [identity, ai](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "ai:use", callback)) {
            return;
        }
        writeRequestLog(request, session);
        modules::AiInteractionQuery query;
        const auto relatedType = request->getParameter("relatedType");
        const auto relatedId = request->getParameter("relatedId");
        if (!relatedType.empty()) {
            query.relatedType = relatedType;
        }
        if (!relatedId.empty()) {
            query.relatedId = relatedId;
        }

        std::optional<int> limit;
        std::optional<int> offset;
        std::string pageError;
        if (!parsePaginationParameter(request, "limit", 1, 100, limit, pageError) ||
            !parsePaginationParameter(request, "offset", 0, 1000000, offset, pageError)) {
            callback(invalidRequest(pageError));
            return;
        }

        const auto interactions = ai->interactions(query);
        Json::Value rows(Json::arrayValue);
        if (!limit && !offset) {
            for (const auto& interaction : interactions) {
                rows.append(aiInteractionToJson(interaction));
            }
            callback(jsonResponse(responseEnvelope(true, "OK", "AI interactions returned", rows)));
            return;
        }

        const auto effectiveLimit = limit.value_or(20);
        const auto effectiveOffset = offset.value_or(0);
        const auto start = std::min<std::size_t>(static_cast<std::size_t>(effectiveOffset), interactions.size());
        const auto end = std::min<std::size_t>(start + static_cast<std::size_t>(effectiveLimit), interactions.size());
        for (auto index = start; index < end; ++index) {
            rows.append(aiInteractionToJson(interactions[index]));
        }

        Json::Value page(Json::objectValue);
        page["items"] = rows;
        page["total"] = static_cast<Json::UInt64>(interactions.size());
        page["limit"] = effectiveLimit;
        page["offset"] = effectiveOffset;
        callback(jsonResponse(responseEnvelope(true, "OK", "AI interactions returned", page)));
    }, {drogon::Get});
}

}  // namespace

int runDrogonServer(const app::AppConfig& config) {
    auto application = std::make_shared<app::Application>(config);

    drogon::orm::DbClientPtr mysqlClient;
    if (config.storage.repositoryStore == "mysql") {
        mysqlClient = data::makeMysqlClient(config.mysql, 2);
    }

    auto identity = createIdentityService(config, mysqlClient);
    auto assets = std::make_shared<modules::AssetService>(createAssetRepository(config, mysqlClient));
    auto monitoring = std::make_shared<modules::MonitoringService>(createRuntimeStateRepository(config, mysqlClient));
    auto alerts = std::make_shared<modules::AlertService>(createAlertRepository(config, mysqlClient));
    auto maintenance = std::make_shared<modules::MaintenanceService>(createWorkOrderRepository(config, mysqlClient));
    auto ai = std::make_shared<modules::AiService>(config.ai, createAiInteractionRepository(config, mysqlClient));

    application->start();
    registerRoutes(application, identity, assets, monitoring, alerts, maintenance, ai);

    drogon::app().addListener(config.host, config.port).run();
    application->stop();
    return 0;
}

}  // namespace induspilot::http

#endif  // INDUSPILOT_WITH_DROGON
