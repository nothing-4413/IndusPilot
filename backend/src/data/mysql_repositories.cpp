#include "induspilot/data/mysql_repositories.hpp"

#ifdef INDUSPILOT_WITH_DROGON

#include <drogon/orm/Exception.h>
#include <drogon/orm/Result.h>

#include <sstream>
#include <utility>

namespace induspilot::data {
namespace {

std::string mysqlConnInfo(const app::DatabaseConfig& config) {
    if (!config.uri.empty()) {
        return config.uri;
    }

    std::ostringstream out;
    out << "host=" << config.host
        << " port=" << config.port
        << " dbname=" << config.database
        << " user=" << config.user;
    if (!config.password.empty()) {
        out << " password=" << config.password;
    }
    return out.str();
}

std::vector<std::string> splitRoles(const std::string& value) {
    std::vector<std::string> roles;
    std::string current;
    for (const auto ch : value) {
        if (ch == ',') {
            if (!current.empty()) {
                roles.push_back(current);
            }
            current.clear();
            continue;
        }
        current.push_back(ch);
    }
    if (!current.empty()) {
        roles.push_back(current);
    }
    return roles;
}

domain::AssetStatus assetStatusFromString(const std::string& value) {
    if (value == "inactive") {
        return domain::AssetStatus::Inactive;
    }
    if (value == "maintenance") {
        return domain::AssetStatus::Maintenance;
    }
    if (value == "retired") {
        return domain::AssetStatus::Retired;
    }
    return domain::AssetStatus::Active;
}

std::string assetStatusToString(domain::AssetStatus status) {
    switch (status) {
        case domain::AssetStatus::Inactive:
            return "inactive";
        case domain::AssetStatus::Maintenance:
            return "maintenance";
        case domain::AssetStatus::Retired:
            return "retired";
        case domain::AssetStatus::Active:
            return "active";
    }
    return "active";
}

domain::EquipmentAsset assetFromRow(const drogon::orm::Row& row) {
    return domain::EquipmentAsset{
        row["asset_code"].as<std::string>(),
        row["name"].as<std::string>(),
        row["asset_type"].as<std::string>(),
        row["factory"].as<std::string>(),
        row["workshop"].as<std::string>(),
        row["production_line"].as<std::string>(),
        assetStatusFromString(row["status"].as<std::string>())};
}

}  // namespace

drogon::orm::DbClientPtr makeMysqlClient(const app::DatabaseConfig& config, std::size_t connectionCount) {
    return drogon::orm::DbClient::newMysqlClient(mysqlConnInfo(config), connectionCount);
}

MySqlUserRepository::MySqlUserRepository(drogon::orm::DbClientPtr client) : client_(std::move(client)) {}

std::optional<UserCredential> MySqlUserRepository::findByUsername(const std::string& username) const {
    const auto result = client_->execSqlSync(
        "SELECT u.id, u.username, u.password_hash, GROUP_CONCAT(r.code ORDER BY r.code) AS roles "
        "FROM users u "
        "LEFT JOIN user_roles ur ON ur.user_id = u.id "
        "LEFT JOIN roles r ON r.id = ur.role_id "
        "WHERE u.username = ? AND u.enabled = TRUE "
        "GROUP BY u.id, u.username, u.password_hash",
        username);
    if (result.empty()) {
        return std::nullopt;
    }

    const auto row = result[0];
    UserCredential credential;
    credential.user.id = std::to_string(row["id"].as<long long>());
    credential.user.username = row["username"].as<std::string>();
    credential.passwordHash = row["password_hash"].as<std::string>();
    if (!row["roles"].isNull()) {
        credential.user.roles = splitRoles(row["roles"].as<std::string>());
    }
    return credential;
}

std::vector<domain::User> MySqlUserRepository::listUsers() const {
    const auto result = client_->execSqlSync(
        "SELECT u.id, u.username, GROUP_CONCAT(r.code ORDER BY r.code) AS roles "
        "FROM users u "
        "LEFT JOIN user_roles ur ON ur.user_id = u.id "
        "LEFT JOIN roles r ON r.id = ur.role_id "
        "WHERE u.enabled = TRUE "
        "GROUP BY u.id, u.username");

    std::vector<domain::User> users;
    for (const auto& row : result) {
        domain::User user;
        user.id = std::to_string(row["id"].as<long long>());
        user.username = row["username"].as<std::string>();
        if (!row["roles"].isNull()) {
            user.roles = splitRoles(row["roles"].as<std::string>());
        }
        users.push_back(user);
    }
    return users;
}

MySqlPermissionRepository::MySqlPermissionRepository(drogon::orm::DbClientPtr client) : client_(std::move(client)) {}

std::vector<std::string> MySqlPermissionRepository::permissionsForRoles(const std::vector<std::string>& roles) const {
    std::vector<std::string> permissions;
    for (const auto& role : roles) {
        const auto result = client_->execSqlSync(
            "SELECT p.code "
            "FROM permissions p "
            "JOIN role_permissions rp ON rp.permission_id = p.id "
            "JOIN roles r ON r.id = rp.role_id "
            "WHERE r.code = ? "
            "ORDER BY p.code",
            role);
        for (const auto& row : result) {
            permissions.push_back(row["code"].as<std::string>());
        }
    }
    return permissions;
}

MySqlAssetRepository::MySqlAssetRepository(drogon::orm::DbClientPtr client) : client_(std::move(client)) {}

domain::EquipmentAsset MySqlAssetRepository::save(domain::EquipmentAsset asset) {
    client_->execSqlSync(
        "INSERT INTO equipment_assets(asset_code, name, asset_type, factory, workshop, production_line, status) "
        "VALUES(?, ?, ?, ?, ?, ?, ?) "
        "ON DUPLICATE KEY UPDATE name = VALUES(name), asset_type = VALUES(asset_type), factory = VALUES(factory), "
        "workshop = VALUES(workshop), production_line = VALUES(production_line), status = VALUES(status)",
        asset.id,
        asset.name,
        asset.type,
        asset.factory,
        asset.workshop,
        asset.productionLine,
        assetStatusToString(asset.status));
    return asset;
}

std::vector<domain::EquipmentAsset> MySqlAssetRepository::list() const {
    const auto result = client_->execSqlSync(
        "SELECT asset_code, name, asset_type, factory, workshop, production_line, status "
        "FROM equipment_assets ORDER BY asset_code");

    std::vector<domain::EquipmentAsset> assets;
    for (const auto& row : result) {
        assets.push_back(assetFromRow(row));
    }
    return assets;
}

std::optional<domain::EquipmentAsset> MySqlAssetRepository::findById(const std::string& id) const {
    const auto result = client_->execSqlSync(
        "SELECT asset_code, name, asset_type, factory, workshop, production_line, status "
        "FROM equipment_assets WHERE asset_code = ?",
        id);
    if (result.empty()) {
        return std::nullopt;
    }
    return assetFromRow(result[0]);
}

}  // namespace induspilot::data

#endif  // INDUSPILOT_WITH_DROGON