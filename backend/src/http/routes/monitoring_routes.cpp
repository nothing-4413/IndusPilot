#include "induspilot/http/route_registrars.hpp"

#include "induspilot/http/http_common.hpp"

namespace induspilot::http {
namespace {

Json::Value runtimeStateToJson(const modules::RuntimeState& state) {
    Json::Value value;
    value["assetId"] = state.assetId;
    value["state"] = state.state;
    value["metricSummary"] = state.metricSummary;
    value["updatedAt"] = state.updatedAt;
    value["severity"] = state.severity;
    return value;
}

}  // namespace

void registerMonitoringRoutes(drogon::HttpAppFramework& server, const HttpServerContext& context) {
    const auto& identity = context.identity;
    const auto& monitoring = context.monitoring;

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
}

}  // namespace induspilot::http
