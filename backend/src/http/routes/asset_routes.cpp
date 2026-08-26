#include "induspilot/http/route_registrars.hpp"

#include "induspilot/http/http_common.hpp"

namespace induspilot::http {
namespace {

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

}  // namespace

void registerAssetRoutes(drogon::HttpAppFramework& server, const HttpServerContext& context) {
    const auto& identity = context.identity;
    const auto& assets = context.assets;

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
}

}  // namespace induspilot::http
