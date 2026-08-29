#include "induspilot/http/route_registrars.hpp"

#include "induspilot/http/http_common.hpp"

namespace induspilot::http {
namespace {

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

Json::Value alertRuleToJson(const domain::AlertRule& rule) {
    Json::Value value;
    value["id"] = rule.id;
    value["name"] = rule.name;
    value["assetId"] = rule.assetId;
    value["minSeverity"] = rule.minSeverity;
    value["channel"] = rule.channel;
    value["target"] = rule.target;
    value["enabled"] = rule.enabled;
    return value;
}

Json::Value alertNotificationToJson(const domain::AlertNotification& notification) {
    Json::Value value;
    value["id"] = notification.id;
    value["alertId"] = notification.alertId;
    value["ruleId"] = notification.ruleId;
    value["channel"] = notification.channel;
    value["target"] = notification.target;
    value["status"] = notification.status;
    value["message"] = notification.message;
    value["attemptCount"] = notification.attemptCount;
    value["lastError"] = notification.lastError;
    value["deliveredAt"] = notification.deliveredAt;
    value["nextAttemptAtUnixMs"] = Json::Int64(notification.nextAttemptAtUnixMs);
    value["leaseUntilUnixMs"] = Json::Int64(notification.leaseUntilUnixMs);
    value["maxAttempts"] = notification.maxAttempts;
    return value;
}

Json::Value notificationDispatchSummaryToJson(const modules::NotificationDispatchSummary& summary) {
    Json::Value value;
    value["sent"] = summary.sent;
    value["failed"] = summary.failed;
    value["skipped"] = summary.skipped;
    return value;
}

std::optional<modules::AlertQuery> alertQueryFromRequest(
    const drogon::HttpRequestPtr& request,
    std::string& error) {
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

}  // namespace

void registerAlertRoutes(drogon::HttpAppFramework& server, const HttpServerContext& context) {
    const auto& identity = context.identity;
    const auto& alerts = context.alerts;
    const auto& audit = context.audit;

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

    server.registerHandler("/api/v1/alert-rules", [identity, alerts](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "alert:read", callback)) {
            return;
        }
        writeRequestLog(request, session);
        Json::Value rows(Json::arrayValue);
        for (const auto& rule : alerts->rules()) {
            rows.append(alertRuleToJson(rule));
        }
        callback(jsonResponse(responseEnvelope(true, "OK", "alert rules returned", rows)));
    }, {drogon::Get});

    server.registerHandler("/api/v1/alert-rules", [identity, alerts](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "alert:write", callback)) {
            return;
        }
        writeRequestLog(request, session);
        const auto payload = request->getJsonObject();
        if (!payload || !payload->isMember("id") || !payload->isMember("name") || !payload->isMember("minSeverity") ||
            !payload->isMember("channel") || !payload->isMember("target")) {
            callback(invalidRequest("id, name, minSeverity, channel and target are required"));
            return;
        }
        const auto minSeverity = (*payload)["minSeverity"].asString();
        if (!modules::alertSeverityFromString(minSeverity)) {
            callback(invalidRequest("unsupported alert severity"));
            return;
        }
        const auto rule = alerts->createRule(domain::AlertRule{
            (*payload)["id"].asString(),
            (*payload)["name"].asString(),
            payload->isMember("assetId") ? (*payload)["assetId"].asString() : "",
            minSeverity,
            (*payload)["channel"].asString(),
            (*payload)["target"].asString(),
            payload->isMember("enabled") ? (*payload)["enabled"].asBool() : true});
        callback(jsonResponse(responseEnvelope(true, "OK", "alert rule created", alertRuleToJson(rule))));
    }, {drogon::Post});

    server.registerHandler("/api/v1/alert-notifications", [identity, alerts](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "alert:read", callback)) {
            return;
        }
        writeRequestLog(request, session);
        Json::Value rows(Json::arrayValue);
        for (const auto& notification : alerts->notifications()) {
            rows.append(alertNotificationToJson(notification));
        }
        callback(jsonResponse(responseEnvelope(true, "OK", "alert notifications returned", rows)));
    }, {drogon::Get});

    server.registerHandler("/api/v1/alert-notifications/dispatch", [identity, alerts, audit](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "alert:write", callback)) {
            return;
        }
        writeRequestLog(request, session);
        const auto summary = alerts->dispatchQueuedNotifications();
        const auto result = summary.failed > 0 ? "partial_failed" : "success";
        const auto resourceId = "sent=" + std::to_string(summary.sent) + ";failed=" + std::to_string(summary.failed) + ";skipped=" + std::to_string(summary.skipped);
        recordAuditEvent(audit, session->user.username, "alert-notification.dispatch", "alert-notification-batch", resourceId, result, traceIdFor(request));
        callback(jsonResponse(responseEnvelope(true, "OK", "alert notifications dispatched", notificationDispatchSummaryToJson(summary))));
    }, {drogon::Post});

    server.registerHandler("/api/v1/alert-notifications/{1}/retry", [identity, alerts, audit](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback, const std::string& notificationId) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "alert:write", callback)) {
            return;
        }
        writeRequestLog(request, session);
        const auto notification = alerts->retryNotification(notificationId);
        if (!notification) {
            callback(notFound("alert notification not found"));
            return;
        }
        recordAuditEvent(audit, session->user.username, "alert-notification.retry", "alert-notification", notificationId, notification->status, traceIdFor(request));
        callback(jsonResponse(responseEnvelope(true, "OK", "alert notification retried", alertNotificationToJson(*notification))));
    }, {drogon::Post});

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
}

}  // namespace induspilot::http
