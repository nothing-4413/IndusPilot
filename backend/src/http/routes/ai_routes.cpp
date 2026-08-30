#include "induspilot/http/route_registrars.hpp"

#include "induspilot/http/http_common.hpp"

#include <algorithm>
#include <exception>
#include <optional>
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

}  // namespace

void registerAiRoutes(drogon::HttpAppFramework& server, const HttpServerContext& context) {
    const auto& identity = context.identity;
    const auto& ai = context.ai;

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
        try {
            const auto suggestion = ai->troubleshoot(*aiRequest);
            callback(jsonResponse(responseEnvelope(true, "OK", "AI troubleshooting returned", aiSuggestionToJson(suggestion))));
        } catch (const std::exception& exception) {
            callback(dependencyUnavailable(std::string("AI interaction storage operation failed: ") + exception.what()));
        }
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
        try {
            const auto suggestion = ai->summarizeLogs(*aiRequest);
            callback(jsonResponse(responseEnvelope(true, "OK", "AI log summary returned", aiSuggestionToJson(suggestion))));
        } catch (const std::exception& exception) {
            callback(dependencyUnavailable(std::string("AI interaction storage operation failed: ") + exception.what()));
        }
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
        try {
            const auto diagnosis = ai->diagnose(*diagnosisRequest);
            callback(jsonResponse(responseEnvelope(true, "OK", "AI diagnosis returned", diagnosisResultToJson(diagnosis))));
        } catch (const std::exception& exception) {
            callback(dependencyUnavailable(std::string("AI interaction storage operation failed: ") + exception.what()));
        }
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

        const auto paged = limit.has_value() || offset.has_value();
        if (paged) {
            query.limit = static_cast<std::size_t>(limit.value_or(20));
            query.offset = static_cast<std::size_t>(offset.value_or(0));
        }
        modules::AiInteractionPage interactions;
        try {
            interactions = ai->interactionsPage(query);
        } catch (const std::exception& exception) {
            callback(dependencyUnavailable(std::string("AI interaction storage operation failed: ") + exception.what()));
            return;
        }
        Json::Value rows(Json::arrayValue);
        if (!paged) {
            for (const auto& interaction : interactions.interactions) {
                rows.append(aiInteractionToJson(interaction));
            }
            callback(jsonResponse(responseEnvelope(true, "OK", "AI interactions returned", rows)));
            return;
        }

        for (const auto& interaction : interactions.interactions) {
            rows.append(aiInteractionToJson(interaction));
        }

        Json::Value page(Json::objectValue);
        page["items"] = rows;
        page["total"] = static_cast<Json::UInt64>(interactions.total);
        page["limit"] = static_cast<Json::UInt64>(*query.limit);
        page["offset"] = static_cast<Json::UInt64>(query.offset);
        callback(jsonResponse(responseEnvelope(true, "OK", "AI interactions returned", page)));
    }, {drogon::Get});
}

}  // namespace induspilot::http
