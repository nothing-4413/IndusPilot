#include "induspilot/http/route_registrars.hpp"

#include "induspilot/http/http_common.hpp"

namespace induspilot::http {
namespace {

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

}  // namespace

void registerAuthRoutes(drogon::HttpAppFramework& server, const HttpServerContext& context) {
    const auto& identity = context.identity;
    const auto& audit = context.audit;

    server.registerHandler("/api/v1/auth/login", [identity, audit](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        writeRequestLog(request);
        const auto payload = request->getJsonObject();
        if (!payload || !payload->isMember("username") || !payload->isMember("password")) {
            callback(invalidRequest("username and password are required"));
            return;
        }

        const auto username = (*payload)["username"].asString();
        const auto result = identity->login({username, (*payload)["password"].asString()});
        if (!result.success || !result.session) {
            const auto code = result.code.empty() ? "AUTHENTICATION_FAILED" : result.code;
            if (code == "AUTHENTICATION_LOCKED") {
                auto response = jsonResponse(responseEnvelope(false, code, result.message), drogon::k429TooManyRequests);
                response->addHeader("Retry-After", std::to_string(result.retryAfterSeconds));
                recordAuditEvent(audit, username, "auth.login.locked", "user", username, "locked", traceIdFor(request));
                callback(response);
                return;
            }
            recordAuditEvent(audit, username, "auth.login.failed", "user", username, "failed", traceIdFor(request));
            callback(jsonResponse(responseEnvelope(false, code, "invalid username or password"), drogon::k401Unauthorized));
            return;
        }

        writeRequestLog(request, result.session);
        recordAuditEvent(audit, result.session->user.username, "auth.login", "user", result.session->user.id, "success", traceIdFor(request));
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

    server.registerHandler("/api/v1/auth/password", [identity, audit](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        const auto session = requireSession(identity, request, callback);
        if (!session) {
            return;
        }

        const auto payload = request->getJsonObject();
        if (!payload || !payload->isMember("currentPassword") || !payload->isMember("newPassword")) {
            callback(invalidRequest("currentPassword and newPassword are required"));
            return;
        }

        const auto result = identity->changePassword(
            session->user.username,
            (*payload)["currentPassword"].asString(),
            (*payload)["newPassword"].asString());
        if (!result.success) {
            const auto auditResult = result.code == "CURRENT_PASSWORD_INVALID"
                ? "invalid_current_password"
                : result.code == "PASSWORD_POLICY_VIOLATION"
                    ? "invalid_new_password"
                    : "storage_failed";
            recordAuditEvent(
                audit,
                session->user.username,
                "auth.password.change.failed",
                "user",
                session->user.id,
                auditResult,
                traceIdFor(request));
            if (result.code == "CURRENT_PASSWORD_INVALID") {
                callback(jsonResponse(
                    responseEnvelope(false, result.code, "current password is invalid"),
                    drogon::k401Unauthorized));
                return;
            }
            if (result.code == "PASSWORD_POLICY_VIOLATION") {
                callback(jsonResponse(
                    responseEnvelope(false, result.code, result.message),
                    drogon::k400BadRequest));
                return;
            }
            callback(jsonResponse(
                responseEnvelope(false, "PASSWORD_UPDATE_FAILED", "password update failed"),
                drogon::k503ServiceUnavailable));
            return;
        }

        writeRequestLog(request, session);
        recordAuditEvent(
            audit,
            session->user.username,
            "auth.password.changed",
            "user",
            session->user.id,
            "success",
            traceIdFor(request));
        callback(jsonResponse(responseEnvelope(true, "OK", "password changed")));
    }, {drogon::Post});

    server.registerHandler("/api/v1/auth/logout", [identity](const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        if (!identity->logout(bearerToken(request))) {
            callback(unauthorized());
            return;
        }
        writeRequestLog(request);
        callback(jsonResponse(responseEnvelope(true, "OK", "logout succeeded")));
    }, {drogon::Post});
}

}  // namespace induspilot::http
