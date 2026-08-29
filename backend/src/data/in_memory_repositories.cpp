#include "induspilot/data/in_memory_repositories.hpp"

#include <algorithm>
#include <utility>

namespace induspilot::data {

InMemoryUserRepository::InMemoryUserRepository() {
    std::lock_guard<std::mutex> lock(mutex_);
    users_["admin"] = UserCredential{domain::User{"user-admin", "admin", {"admin"}}, "plain:admin123", 1, true};
    users_["operator"] = UserCredential{domain::User{"user-operator", "operator", {"operator"}}, "plain:operator123", 1, true};
    users_["maintainer"] = UserCredential{domain::User{"user-maintainer", "maintainer", {"maintainer"}}, "plain:maintainer123", 1, true};
}

std::optional<UserCredential> InMemoryUserRepository::findByUsername(const std::string& username) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = users_.find(username);
    if (it == users_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<domain::User> InMemoryUserRepository::listUsers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<domain::User> users;
    for (const auto& item : users_) {
        users.push_back(item.second.user);
    }
    return users;
}

bool InMemoryUserRepository::updatePasswordHash(const std::string& username, const std::string& passwordHash) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = users_.find(username);
    if (it == users_.end()) {
        return false;
    }
    it->second.passwordHash = passwordHash;
    ++it->second.credentialVersion;
    it->second.requiresPasswordRotation = false;
    return true;
}

InMemoryPermissionRepository::InMemoryPermissionRepository() {
    rolePermissions_["admin"] = {"asset:read", "asset:write", "monitoring:write", "alert:read", "alert:write", "work-order:read", "work-order:write", "ai:use", "audit:read", "audit:export"};
    rolePermissions_["operator"] = {"asset:read", "monitoring:write", "alert:read", "alert:write", "work-order:read", "ai:use"};
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

domain::AlertRule InMemoryAlertRepository::saveRule(domain::AlertRule rule) {
    rules_[rule.id] = rule;
    return rule;
}

std::vector<domain::AlertRule> InMemoryAlertRepository::listRules() const {
    std::vector<domain::AlertRule> rules;
    for (const auto& item : rules_) {
        rules.push_back(item.second);
    }
    return rules;
}

domain::AlertNotification InMemoryAlertRepository::saveNotification(domain::AlertNotification notification) {
    std::lock_guard<std::mutex> lock(notificationsMutex_);
    notifications_[notification.id] = notification;
    return notification;
}

std::vector<domain::AlertNotification> InMemoryAlertRepository::listNotifications() const {
    std::lock_guard<std::mutex> lock(notificationsMutex_);
    std::vector<domain::AlertNotification> notifications;
    for (const auto& item : notifications_) {
        notifications.push_back(item.second);
    }
    return notifications;
}

std::vector<domain::AlertNotification> InMemoryAlertRepository::claimDueNotifications(
    std::int64_t nowUnixMs,
    std::int64_t leaseUntilUnixMs,
    int limit,
    const std::string& leaseToken) {
    std::lock_guard<std::mutex> lock(notificationsMutex_);
    std::vector<domain::AlertNotification> claimed;
    for (auto& item : notifications_) {
        auto& notification = item.second;
        const bool statusEligible = notification.status == "queued" || notification.status == "retrying" ||
                                    (notification.status == "delivering" && notification.leaseUntilUnixMs <= nowUnixMs);
        if (static_cast<int>(claimed.size()) >= limit || !statusEligible ||
            notification.nextAttemptAtUnixMs > nowUnixMs ||
            (notification.leaseUntilUnixMs > nowUnixMs && !notification.leaseToken.empty())) {
            continue;
        }
        notification.status = "delivering";
        notification.leaseUntilUnixMs = leaseUntilUnixMs;
        notification.leaseToken = leaseToken;
        claimed.push_back(notification);
    }
    return claimed;
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

domain::WorkOrderAttachment InMemoryWorkOrderRepository::saveAttachment(domain::WorkOrderAttachment attachment) {
    attachments_[attachment.id] = attachment;
    return attachment;
}

std::vector<domain::WorkOrderAttachment> InMemoryWorkOrderRepository::listAttachments(const std::string& workOrderId) const {
    std::vector<domain::WorkOrderAttachment> attachments;
    for (const auto& item : attachments_) {
        if (item.second.workOrderId == workOrderId) {
            attachments.push_back(item.second);
        }
    }
    return attachments;
}
domain::RuntimeState InMemoryRuntimeStateRepository::save(domain::RuntimeState state) {
    states_[state.assetId] = state;
    return state;
}

std::vector<domain::RuntimeState> InMemoryRuntimeStateRepository::list() const {
    std::vector<domain::RuntimeState> states;
    for (const auto& item : states_) {
        states.push_back(item.second);
    }
    return states;
}

std::optional<domain::RuntimeState> InMemoryRuntimeStateRepository::findByAssetId(const std::string& assetId) const {
    const auto it = states_.find(assetId);
    if (it == states_.end()) {
        return std::nullopt;
    }
    return it->second;
}
domain::OperationAuditEvent InMemoryOperationAuditRepository::save(domain::OperationAuditEvent event) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto existing = std::find_if(events_.begin(), events_.end(), [&event](const auto& item) { return item.id == event.id; });
    if (existing != events_.end()) {
        *existing = event;
    } else {
        events_.push_back(event);
    }
    return event;
}

std::vector<domain::OperationAuditEvent> InMemoryOperationAuditRepository::list() const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto events = events_;
    std::reverse(events.begin(), events.end());
    return events;
}

std::optional<domain::OperationAuditEvent> InMemoryOperationAuditRepository::latest() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (events_.empty()) {
        return std::nullopt;
    }
    return events_.back();
}

std::vector<domain::OperationAuditEvent> InMemoryOperationAuditRepository::listForIntegrity() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return events_;
}
domain::AiInteraction InMemoryAiInteractionRepository::save(domain::AiInteraction interaction) {
    interactions_.push_back(interaction);
    return interaction;
}

std::vector<domain::AiInteraction> InMemoryAiInteractionRepository::list() const {
    return interactions_;
}

}  // namespace induspilot::data
