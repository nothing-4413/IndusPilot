#include "induspilot/http/route_registrars.hpp"

#include "induspilot/http/http_common.hpp"

namespace induspilot::http {
namespace {

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

std::optional<modules::WorkOrderQuery> workOrderQueryFromRequest(
    const drogon::HttpRequestPtr& request,
    std::string& error) {
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

}  // namespace

void registerWorkOrderRoutes(drogon::HttpAppFramework& server, const HttpServerContext& context) {
    const auto& identity = context.identity;
    const auto& alerts = context.alerts;
    const auto& maintenance = context.maintenance;

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
}

}  // namespace induspilot::http
