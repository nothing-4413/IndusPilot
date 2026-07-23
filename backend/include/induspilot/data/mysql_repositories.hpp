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

class MySqlAlertRepository final : public AlertRepository {
public:
    explicit MySqlAlertRepository(drogon::orm::DbClientPtr client);

    domain::Alert save(domain::Alert alert) override;
    std::vector<domain::Alert> list() const override;
    std::optional<domain::Alert> findById(const std::string& id) const override;

private:
    drogon::orm::DbClientPtr client_;
};

class MySqlWorkOrderRepository final : public WorkOrderRepository {
public:
    explicit MySqlWorkOrderRepository(drogon::orm::DbClientPtr client);

    domain::WorkOrder save(domain::WorkOrder order) override;
    std::vector<domain::WorkOrder> list() const override;
    std::optional<domain::WorkOrder> findById(const std::string& id) const override;
    std::vector<domain::WorkOrder> historyForAsset(const std::string& assetId) const override;
    domain::WorkOrderAttachment saveAttachment(domain::WorkOrderAttachment attachment) override;
    std::vector<domain::WorkOrderAttachment> listAttachments(const std::string& workOrderId) const override;

private:
    drogon::orm::DbClientPtr client_;
};

class MySqlRuntimeStateRepository final : public RuntimeStateRepository {
public:
    explicit MySqlRuntimeStateRepository(drogon::orm::DbClientPtr client);

    domain::RuntimeState save(domain::RuntimeState state) override;
    std::vector<domain::RuntimeState> list() const override;
    std::optional<domain::RuntimeState> findByAssetId(const std::string& assetId) const override;

private:
    drogon::orm::DbClientPtr client_;
};

class MySqlAiInteractionRepository final : public AiInteractionRepository {
public:
    explicit MySqlAiInteractionRepository(drogon::orm::DbClientPtr client);

    domain::AiInteraction save(domain::AiInteraction interaction) override;
    std::vector<domain::AiInteraction> list() const override;

private:
    drogon::orm::DbClientPtr client_;
};

drogon::orm::DbClientPtr makeMysqlClient(const app::DatabaseConfig& config, std::size_t connectionCount = 1);

#endif  // INDUSPILOT_WITH_DROGON

}  // namespace induspilot::data