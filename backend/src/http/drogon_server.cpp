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

Json::Value runtimeStateToJson(const modules::RuntimeState& state) {
    Json::Value value;
    value["assetId"] = state.assetId;
    value["state"] = state.state;
    value["metricSummary"] = state.metricSummary;
    value["updatedAt"] = state.updatedAt;
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
void registerRoutes(
    const std::shared_ptr<app::Application>& application,
    const std::shared_ptr<modules::IdentityService>& identity,
    const std::shared_ptr<modules::AssetService>& assets,
    const std::shared_ptr<modules::MonitoringService>& monitoring,
    const std::shared_ptr<modules::AlertService>& alerts,
    const std::shared_ptr<modules::MaintenanceService>& maintenance,
    const std::shared_ptr<modules::AiService>& ai) {
    auto& server = drogon::app();

    server.registerHandler("/health", [application](const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
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
        const auto payload = request->getJsonObject();
        if (!payload || !payload->isMember("username") || !payload->isMember("password")) {
            callback(jsonResponse(responseEnvelope(false, "INVALID_REQUEST", "username and password are required"), drogon::k400BadRequest));
            return;
        }

        const auto result = identity->login({(*payload)["username"].asString(), (*payload)["password"].asString()});
        if (!result.success || !result.session) {
            callback(jsonResponse(responseEnvelope(false, "AUTHENTICATION_FAILED", "invalid username or password"), drogon::k401Unauthorized));
            return;
        }

        callback(jsonResponse(responseEnvelope(true, "OK", "login succeeded", sessionToJson(*result.session))));
    }, {drogon::Post});

    server.registerHandler("/api/v1/auth/session", [identity](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        const auto session = identity->validateSession(bearerToken(request));
        if (!session) {
            callback(jsonResponse(responseEnvelope(false, "AUTHENTICATION_REQUIRED", "session is missing or expired"), drogon::k401Unauthorized));
            return;
        }
        callback(jsonResponse(responseEnvelope(true, "OK", "session is valid", sessionToJson(*session))));
    }, {drogon::Get});

    server.registerHandler("/api/v1/auth/logout", [identity](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        if (!identity->logout(bearerToken(request))) {
            callback(jsonResponse(responseEnvelope(false, "AUTHENTICATION_REQUIRED", "session is missing or expired"), drogon::k401Unauthorized));
            return;
        }
        callback(jsonResponse(responseEnvelope(true, "OK", "logout succeeded")));
    }, {drogon::Post});

    server.registerHandler("/api/v1/assets", [assets](const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        Json::Value rows(Json::arrayValue);
        for (const auto& asset : assets->list()) {
            rows.append(assetToJson(asset));
        }
        callback(jsonResponse(responseEnvelope(true, "OK", "assets returned", rows)));
    }, {drogon::Get});

    server.registerHandler("/api/v1/monitoring/states", [monitoring](const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        Json::Value data;
        const auto summary = monitoring->summarizeStates();
        Json::Value summaryJson;
        for (const auto& item : summary) {
            summaryJson[item.first] = item.second;
        }
        data["summary"] = summaryJson;
        callback(jsonResponse(responseEnvelope(true, "OK", "monitoring summary returned", data)));
    }, {drogon::Get});

    server.registerHandler("/api/v1/alerts", [alerts](const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        Json::Value rows(Json::arrayValue);
        for (const auto& alert : alerts->list()) {
            rows.append(alertToJson(alert));
        }
        callback(jsonResponse(responseEnvelope(true, "OK", "alerts returned", rows)));
    }, {drogon::Get});

    server.registerHandler("/api/v1/work-orders", [maintenance](const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        Json::Value rows(Json::arrayValue);
        for (const auto& order : maintenance->list()) {
            rows.append(workOrderToJson(order));
        }
        callback(jsonResponse(responseEnvelope(true, "OK", "work orders returned", rows)));
    }, {drogon::Get});

    server.registerHandler("/api/v1/ai/status", [ai](const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
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
