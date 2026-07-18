#pragma once

#include "induspilot/data/repositories.hpp"

#include <map>
#include <string>
#include <vector>

namespace induspilot::data {

class InMemoryUserRepository final : public UserRepository {
public:
    InMemoryUserRepository();

    std::optional<UserCredential> findByUsername(const std::string& username) const override;
    std::vector<domain::User> listUsers() const override;

private:
    std::map<std::string, UserCredential> users_;
};

class InMemoryPermissionRepository final : public PermissionRepository {
public:
    InMemoryPermissionRepository();

    std::vector<std::string> permissionsForRoles(const std::vector<std::string>& roles) const override;

private:
    std::map<std::string, std::vector<std::string>> rolePermissions_;
};

class InMemoryAssetRepository final : public AssetRepository {
public:
    InMemoryAssetRepository();

    domain::EquipmentAsset save(domain::EquipmentAsset asset) override;
    std::vector<domain::EquipmentAsset> list() const override;
    std::optional<domain::EquipmentAsset> findById(const std::string& id) const override;

private:
    std::map<std::string, domain::EquipmentAsset> assets_;
};

class InMemoryAlertRepository final : public AlertRepository {
public:
    domain::Alert save(domain::Alert alert) override;
    std::vector<domain::Alert> list() const override;
    std::optional<domain::Alert> findById(const std::string& id) const override;

private:
    std::map<std::string, domain::Alert> alerts_;
};

class InMemoryWorkOrderRepository final : public WorkOrderRepository {
public:
    domain::WorkOrder save(domain::WorkOrder order) override;
    std::vector<domain::WorkOrder> list() const override;
    std::optional<domain::WorkOrder> findById(const std::string& id) const override;
    std::vector<domain::WorkOrder> historyForAsset(const std::string& assetId) const override;

private:
    std::map<std::string, domain::WorkOrder> orders_;
};

class InMemoryAiInteractionRepository final : public AiInteractionRepository {
public:
    domain::AiInteraction save(domain::AiInteraction interaction) override;
    std::vector<domain::AiInteraction> list() const override;

private:
    std::vector<domain::AiInteraction> interactions_;
};

}  // namespace induspilot::data