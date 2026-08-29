#include "induspilot/http/route_registrars.hpp"

#include "induspilot/http/http_common.hpp"

#include <cctype>
#include <sstream>

namespace induspilot::http {
namespace {

bool validAuditTimestamp(const std::string& value) {
    if (value.size() != 19 || value[4] != '-' || value[7] != '-' || value[10] != 'T' ||
        value[13] != ':' || value[16] != ':') {
        return false;
    }
    for (std::size_t index = 0; index < value.size(); ++index) {
        if (index == 4 || index == 7 || index == 10 || index == 13 || index == 16) {
            continue;
        }
        if (!std::isdigit(static_cast<unsigned char>(value[index]))) {
            return false;
        }
    }
    const auto number = [&value](std::size_t offset, std::size_t length) {
        return std::stoi(value.substr(offset, length));
    };
    const auto month = number(5, 2);
    const auto day = number(8, 2);
    const auto hour = number(11, 2);
    const auto minute = number(14, 2);
    const auto second = number(17, 2);
    return month >= 1 && month <= 12 && day >= 1 && day <= 31 && hour <= 23 && minute <= 59 && second <= 59;
}

bool auditQueryFromRequest(const drogon::HttpRequestPtr& request, modules::OperationAuditQuery& query, std::string& error) {
    const auto actor = request->getParameter("actor");
    const auto action = request->getParameter("action");
    const auto resourceType = request->getParameter("resourceType");
    const auto result = request->getParameter("result");
    const auto occurredFrom = request->getParameter("occurredFrom");
    const auto occurredTo = request->getParameter("occurredTo");
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
    if (!occurredFrom.empty() && !validAuditTimestamp(occurredFrom)) {
        error = "occurredFrom must use YYYY-MM-DDTHH:MM:SS";
        return false;
    }
    if (!occurredTo.empty() && !validAuditTimestamp(occurredTo)) {
        error = "occurredTo must use YYYY-MM-DDTHH:MM:SS";
        return false;
    }
    if (!occurredFrom.empty()) {
        query.occurredFrom = occurredFrom;
    }
    if (!occurredTo.empty()) {
        query.occurredTo = occurredTo;
    }
    if (query.occurredFrom && query.occurredTo && *query.occurredFrom > *query.occurredTo) {
        error = "occurredFrom must not be later than occurredTo";
        return false;
    }
    return true;
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

}  // namespace

void registerAuditRoutes(drogon::HttpAppFramework& server, const HttpServerContext& context) {
    const auto& identity = context.identity;
    const auto& audit = context.audit;

    server.registerHandler("/api/v1/audit/events", [identity, audit](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        const auto session = requireSession(identity, request, callback);
        if (!session || !requirePermission(identity, *session, "audit:read", callback)) {
            return;
        }
        writeRequestLog(request, session);
        modules::OperationAuditQuery query;
        std::string queryError;
        if (!auditQueryFromRequest(request, query, queryError)) {
            callback(invalidRequest(queryError));
            return;
        }

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
        modules::OperationAuditQuery query;
        std::string queryError;
        if (!auditQueryFromRequest(request, query, queryError)) {
            callback(invalidRequest(queryError));
            return;
        }
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
}

}  // namespace induspilot::http
