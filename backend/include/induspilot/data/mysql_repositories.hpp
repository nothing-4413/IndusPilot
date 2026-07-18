#pragma once

#include "induspilot/app/config.hpp"
#include "induspilot/data/repositories.hpp"

#ifdef INDUSPILOT_WITH_DROGON
#include <drogon/orm/DbClient.h>
#endif

#include <memory>
#include <string>

namespace induspilot::data {

#ifdef INDUSPILOT_WITH_DROGON

class MySqlUserRepository final : public UserRepository {
public:
    explicit MySqlUserRepository(drogon::orm::DbClientPtr client);

    std::optional<UserCredential> findByUsername(const std::string& username) const override;
    std::vector<domain::User> listUsers() const override;

private:
    drogon::orm::DbClientPtr client_;
};

class MySqlPermissionRepository final : public PermissionRepository {
public:
    explicit MySqlPermissionRepository(drogon::orm::DbClientPtr client);

    std::vector<std::string> permissionsForRoles(const std::vector<std::string>& roles) const override;

private:
    drogon::orm::DbClientPtr client_;
};

class MySqlAssetRepository final : public AssetRepository {
public:
    explicit MySqlAssetRepository(drogon::orm::DbClientPtr client);

    domain::EquipmentAsset save(domain::EquipmentAsset asset) override;
    std::vector<domain::EquipmentAsset> list() const override;
    std::optional<domain::EquipmentAsset> findById(const std::string& id) const override;

private:
    drogon::orm::DbClientPtr client_;
};

drogon::orm::DbClientPtr makeMysqlClient(const app::DatabaseConfig& config, std::size_t connectionCount = 1);

#endif  // INDUSPILOT_WITH_DROGON

}  // namespace induspilot::data