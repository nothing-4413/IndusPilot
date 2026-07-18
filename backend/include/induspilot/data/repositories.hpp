#pragma once

#include "induspilot/domain/domain_types.hpp"

#include <optional>
#include <string>
#include <vector>

namespace induspilot::data {

struct UserCredential {
    domain::User user;
    std::string passwordHash;
};

class UserRepository {
public:
    virtual ~UserRepository() = default;

    virtual std::optional<UserCredential> findByUsername(const std::string& username) const = 0;
    virtual std::vector<domain::User> listUsers() const = 0;
};

class PermissionRepository {
public:
    virtual ~PermissionRepository() = default;

    virtual std::vector<std::string> permissionsForRoles(const std::vector<std::string>& roles) const = 0;
};

class AssetRepository {
public:
    virtual ~AssetRepository() = default;

    virtual domain::EquipmentAsset save(domain::EquipmentAsset asset) = 0;
    virtual std::vector<domain::EquipmentAsset> list() const = 0;
    virtual std::optional<domain::EquipmentAsset> findById(const std::string& id) const = 0;
};

class AlertRepository {
public:
    virtual ~AlertRepository() = default;

    virtual domain::Alert save(domain::Alert alert) = 0;
    virtual std::vector<domain::Alert> list() const = 0;
    virtual std::optional<domain::Alert> findById(const std::string& id) const = 0;
};

class WorkOrderRepository {
public:
    virtual ~WorkOrderRepository() = default;

    virtual domain::WorkOrder save(domain::WorkOrder order) = 0;
    virtual std::vector<domain::WorkOrder> list() const = 0;
    virtual std::optional<domain::WorkOrder> findById(const std::string& id) const = 0;
    virtual std::vector<domain::WorkOrder> historyForAsset(const std::string& assetId) const = 0;
};

class AiInteractionRepository {
public:
    virtual ~AiInteractionRepository() = default;

    virtual domain::AiInteraction save(domain::AiInteraction interaction) = 0;
    virtual std::vector<domain::AiInteraction> list() const = 0;
};

}  // namespace induspilot::data