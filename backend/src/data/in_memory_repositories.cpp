#include "induspilot/data/in_memory_repositories.hpp"

#include <algorithm>
#include <utility>

namespace induspilot::data {

InMemoryUserRepository::InMemoryUserRepository() {
    users_["admin"] = UserCredential{domain::User{"user-admin", "admin", {"admin"}}, "admin123"};
    users_["operator"] = UserCredential{domain::User{"user-operator", "operator", {"operator"}}, "operator123"};
    users_["maintainer"] = UserCredential{domain::User{"user-maintainer", "maintainer", {"maintainer"}}, "maintainer123"};
}

std::optional<UserCredential> InMemoryUserRepository::findByUsername(const std::string& username) const {
    const auto it = users_.find(username);
    if (it == users_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<domain::User> InMemoryUserRepository::listUsers() const {
    std::vector<domain::User> users;
    for (const auto& item : users_) {
        users.push_back(item.second.user);
    }
    return users;
}

InMemoryPermissionRepository::InMemoryPermissionRepository() {
    rolePermissions_["admin"] = {"asset:read", "asset:write", "alert:read", "alert:write", "work-order:read", "work-order:write", "ai:use"};
    rolePermissions_["operator"] = {"asset:read", "alert:read", "alert:write", "work-order:read", "ai:use"};
    rolePermissions_["maintainer"] = {"asset:read", "alert:read", "work-order:read", "work-order:write", "ai:use"};
}

std::vector<std::string> InMemoryPermissionRepository::permissionsForRoles(const std::vector<std::string>& roles) const {
    std::vector<std::string> permissions;
    for (const auto& role : roles) {
        const auto it = rolePermissions_.find(role);
        if (it == rolePermissions_.end()) {
            continue;
        }
        permissions.insert(permissions.end(), it->second.begin(), it->second.end());
    }
    std::sort(permissions.begin(), permissions.end());
    permissions.erase(std::unique(permissions.begin(), permissions.end()), permissions.end());
    return permissions;
}

InMemoryAssetRepository::InMemoryAssetRepository() {
    save(domain::EquipmentAsset{"asset-001", "main motor", "motor", "demo factory", "workshop-1", "line-1", domain::AssetStatus::Active});
}

domain::EquipmentAsset InMemoryAssetRepository::save(domain::EquipmentAsset asset) {
    assets_[asset.id] = asset;
    return asset;
}

std::vector<domain::EquipmentAsset> InMemoryAssetRepository::list() const {
    std::vector<domain::EquipmentAsset> assets;
    for (const auto& item : assets_) {
        assets.push_back(item.second);
    }
    return assets;
}

std::optional<domain::EquipmentAsset> InMemoryAssetRepository::findById(const std::string& id) const {
    const auto it = assets_.find(id);
    if (it == assets_.end()) {
        return std::nullopt;
    }
    return it->second;
}

domain::Alert InMemoryAlertRepository::save(domain::Alert alert) {
    alerts_[alert.id] = alert;
    return alert;
}

std::vector<domain::Alert> InMemoryAlertRepository::list() const {
    std::vector<domain::Alert> alerts;
    for (const auto& item : alerts_) {
        alerts.push_back(item.second);
    }
    return alerts;
}

std::optional<domain::Alert> InMemoryAlertRepository::findById(const std::string& id) const {
    const auto it = alerts_.find(id);
    if (it == alerts_.end()) {
        return std::nullopt;
    }
    return it->second;
}

domain::WorkOrder InMemoryWorkOrderRepository::save(domain::WorkOrder order) {
    orders_[order.id] = order;
    return order;
}

std::vector<domain::WorkOrder> InMemoryWorkOrderRepository::list() const {
    std::vector<domain::WorkOrder> orders;
    for (const auto& item : orders_) {
        orders.push_back(item.second);
    }
    return orders;
}

std::optional<domain::WorkOrder> InMemoryWorkOrderRepository::findById(const std::string& id) const {
    const auto it = orders_.find(id);
    if (it == orders_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<domain::WorkOrder> InMemoryWorkOrderRepository::historyForAsset(const std::string& assetId) const {
    std::vector<domain::WorkOrder> orders;
    for (const auto& item : orders_) {
        if (item.second.assetId == assetId && item.second.state == domain::WorkOrderState::Closed) {
            orders.push_back(item.second);
        }
    }
    return orders;
}

domain::AiInteraction InMemoryAiInteractionRepository::save(domain::AiInteraction interaction) {
    interactions_.push_back(interaction);
    return interaction;
}

std::vector<domain::AiInteraction> InMemoryAiInteractionRepository::list() const {
    return interactions_;
}

}  // namespace induspilot::data