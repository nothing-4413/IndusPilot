#include "induspilot/http/drogon_server.hpp"

#ifdef INDUSPILOT_WITH_DROGON

#include "induspilot/api/api_types.hpp"
#include "induspilot/app/application.hpp"
#include "induspilot/domain/domain_types.hpp"
#include "induspilot/http/http_common.hpp"
#include "induspilot/http/http_server_context.hpp"
#include "induspilot/http/route_registrars.hpp"
#include "induspilot/modules/ai_service.hpp"
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

Json::Value aiSuggestionToJson(const modules::AiSuggestion& suggestion) {
    Json::Value value;
    value["available"] = suggestion.available;
    value["label"] = suggestion.label;
    value["content"] = suggestion.content;
    return value;
}

modules::OperationAuditQuery auditQueryFromRequest(const drogon::HttpRequestPtr& request) {
    modules::OperationAuditQuery query;
    const auto actor = request->getParameter("actor");
    const auto action = request->getParameter("action");
    const auto resourceType = request->getParameter("resourceType");
    const auto result = request->getParameter("result");
    if (!actor.empty()) {
        query.actor = actor;
    }
    if (!action.empty()) {
        query.action = action;
    }
    if (!resourceType.empty()) {
        query.resourceType = resourceType;
    }
    if (!result.empty()) {
        query.result = result;
    }
    return query;
}

std::string csvCell(std::string value) {
    std::string escaped;
    escaped.reserve(value.size());
    bool quote = false;
    for (const auto ch : value) {
        if (ch == '"') {
            escaped += "\"\"";
            quote = true;
        } else {
            if (ch == ',' || ch == '\n' || ch == '\r') {
                quote = true;
            }
            escaped += ch;
        }
    }
    return quote ? "\"" + escaped + "\"" : escaped;
}

std::string operationAuditEventsToCsv(const std::vector<domain::OperationAuditEvent>& events) {
    std::ostringstream out;
    out << "id,actor,action,resourceType,resourceId,result,traceId,occurredAt,previousHash,eventHash\n";
    for (const auto& event : events) {
        out << csvCell(event.id) << ','
            << csvCell(event.actor) << ','
            << csvCell(event.action) << ','
            << csvCell(event.resourceType) << ','
            << csvCell(event.resourceId) << ','
            << csvCell(event.result) << ','
            << csvCell(event.traceId) << ','
            << csvCell(event.occurredAt) << ','
            << csvCell(event.previousHash) << ','
            << csvCell(event.eventHash) << '\n';
    }
    return out.str();
}
Json::Value operationAuditEventToJson(const domain::OperationAuditEvent& event) {
    Json::Value value;
    value["id"] = event.id;
    value["actor"] = event.actor;
    value["action"] = event.action;
    value["resourceType"] = event.resourceType;
    value["resourceId"] = event.resourceId;
    value["result"] = event.result;
    value["traceId"] = event.traceId;
    value["occurredAt"] = event.occurredAt;
    value["previousHash"] = event.previousHash;
    value["eventHash"] = event.eventHash;
    return value;
}
Json::Value operationAuditIntegrityToJson(const modules::OperationAuditIntegrityReport& report) {
    Json::Value value;
    value["verified"] = report.verified;
    value["total"] = static_cast<Json::UInt64>(report.total);
    value["brokenEventId"] = report.brokenEventId;
    value["latestHash"] = report.latestHash;
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

    server.registerHandler("/api/v1/audit/events", [identity, audit](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "audit:read", callback)) {
            return;
        }
        writeRequestLog(request, session);
        const auto query = auditQueryFromRequest(request);

        std::optional<int> limit;
        std::optional<int> offset;
        std::string pageError;
        if (!parsePaginationParameter(request, "limit", 1, 100, limit, pageError) ||
            !parsePaginationParameter(request, "offset", 0, 1000000, offset, pageError)) {
            callback(invalidRequest(pageError));
            return;
        }

        const auto events = audit->events(query);
        Json::Value rows(Json::arrayValue);
        if (!limit && !offset) {
            for (const auto& event : events) {
                rows.append(operationAuditEventToJson(event));
            }
            callback(jsonResponse(responseEnvelope(true, "OK", "operation audit events returned", rows)));
            return;
        }

        const auto effectiveLimit = limit.value_or(20);
        const auto effectiveOffset = offset.value_or(0);
        const auto start = std::min<std::size_t>(static_cast<std::size_t>(effectiveOffset), events.size());
        const auto end = std::min<std::size_t>(start + static_cast<std::size_t>(effectiveLimit), events.size());
        for (auto index = start; index < end; ++index) {
            rows.append(operationAuditEventToJson(events[index]));
        }

        Json::Value page(Json::objectValue);
        page["items"] = rows;
        page["total"] = static_cast<Json::UInt64>(events.size());
        page["limit"] = effectiveLimit;
        page["offset"] = effectiveOffset;
        callback(jsonResponse(responseEnvelope(true, "OK", "operation audit events returned", page)));
    }, {drogon::Get});
    server.registerHandler("/api/v1/audit/integrity", [identity, audit](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "audit:read", callback)) {
            return;
        }
        writeRequestLog(request, session);
        const auto report = audit->integrityReport();
        callback(jsonResponse(responseEnvelope(true, "OK", "operation audit integrity returned", operationAuditIntegrityToJson(report))));
    }, {drogon::Get});
    server.registerHandler("/api/v1/audit/events/export", [identity, audit](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "audit:export", callback)) {
            return;
        }
        writeRequestLog(request, session);
        const auto query = auditQueryFromRequest(request);
        const auto events = audit->events(query);
        recordAuditEvent(audit, session->user.username, "operation-audit.export", "operation-audit", "count=" + std::to_string(events.size()), "success", traceIdFor(request));
        auto response = drogon::HttpResponse::newHttpResponse();
        response->setStatusCode(drogon::k200OK);
        response->setContentTypeCode(drogon::CT_TEXT_PLAIN);
        response->addHeader("Content-Type", "text/csv; charset=utf-8");
        response->addHeader("Content-Disposition", "attachment; filename=operation-audit.csv");
        response->setBody(operationAuditEventsToCsv(events));
        callback(response);
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
    const auto context = buildHttpServerContext(config);

    context.application->start();
    registerRoutes(context);

    drogon::app().addListener(config.host, config.port).run();
    context.application->stop();
    return 0;
}

}  // namespace induspilot::http

#endif  // INDUSPILOT_WITH_DROGON
