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

domain::AlertSeverity alertSeverityFromString(const std::string& value) {
    if (value == "warning") {
        return domain::AlertSeverity::Warning;
    }
    if (value == "critical") {
        return domain::AlertSeverity::Critical;
    }
    return domain::AlertSeverity::Info;
}

std::string alertSeverityToString(domain::AlertSeverity severity) {
    switch (severity) {
        case domain::AlertSeverity::Warning:
            return "warning";
        case domain::AlertSeverity::Critical:
            return "critical";
        case domain::AlertSeverity::Info:
            return "info";
    }
    return "info";
}

domain::AlertState alertStateFromString(const std::string& value) {
    if (value == "acknowledged") {
        return domain::AlertState::Acknowledged;
    }
    if (value == "assigned") {
        return domain::AlertState::Assigned;
    }
    if (value == "resolved") {
        return domain::AlertState::Resolved;
    }
    if (value == "closed") {
        return domain::AlertState::Closed;
    }
    return domain::AlertState::Open;
}

std::string alertStateToString(domain::AlertState state) {
    switch (state) {
        case domain::AlertState::Acknowledged:
            return "acknowledged";
        case domain::AlertState::Assigned:
            return "assigned";
        case domain::AlertState::Resolved:
            return "resolved";
        case domain::AlertState::Closed:
            return "closed";
        case domain::AlertState::Open:
            return "open";
    }
    return "open";
}

domain::WorkOrderState workOrderStateFromString(const std::string& value) {
    if (value == "assigned") {
        return domain::WorkOrderState::Assigned;
    }
    if (value == "processing") {
        return domain::WorkOrderState::Processing;
    }
    if (value == "completed") {
        return domain::WorkOrderState::Completed;
    }
    if (value == "closed") {
        return domain::WorkOrderState::Closed;
    }
    return domain::WorkOrderState::Created;
}

std::string workOrderStateToString(domain::WorkOrderState state) {
    switch (state) {
        case domain::WorkOrderState::Assigned:
            return "assigned";
        case domain::WorkOrderState::Processing:
            return "processing";
        case domain::WorkOrderState::Completed:
            return "completed";
        case domain::WorkOrderState::Closed:
            return "closed";
        case domain::WorkOrderState::Created:
            return "created";
    }
    return "created";
}

std::string nullableString(const drogon::orm::Row& row, const char* field) {
    if (row[field].isNull()) {
        return {};
    }
    return row[field].as<std::string>();
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

domain::Alert alertFromRow(const drogon::orm::Row& row) {
    return domain::Alert{
        row["alert_code"].as<std::string>(),
        nullableString(row, "asset_code"),
        alertSeverityFromString(row["severity"].as<std::string>()),
        alertStateFromString(row["state"].as<std::string>()),
        row["title"].as<std::string>(),
        nullableString(row, "acknowledged_by_username"),
        nullableString(row, "assigned_to_username")};
}

domain::AlertRule alertRuleFromRow(const drogon::orm::Row& row) {
    return domain::AlertRule{
        row["rule_code"].as<std::string>(),
        row["name"].as<std::string>(),
        nullableString(row, "asset_code"),
        row["min_severity"].as<std::string>(),
        row["channel"].as<std::string>(),
        row["target"].as<std::string>(),
        row["enabled"].as<int>() != 0};
}

domain::AlertNotification alertNotificationFromRow(const drogon::orm::Row& row) {
    return domain::AlertNotification{
        row["notification_code"].as<std::string>(),
        row["alert_code"].as<std::string>(),
        row["rule_code"].as<std::string>(),
        row["channel"].as<std::string>(),
        row["target"].as<std::string>(),
        row["status"].as<std::string>(),
        row["message"].as<std::string>(),
        row["attempt_count"].as<int>(),
        nullableString(row, "last_error"),
        nullableString(row, "delivered_at")};
}
domain::WorkOrder workOrderFromRow(const drogon::orm::Row& row) {
    return domain::WorkOrder{
        row["work_order_code"].as<std::string>(),
        nullableString(row, "asset_code"),
        nullableString(row, "alert_code"),
        workOrderStateFromString(row["state"].as<std::string>()),
        nullableString(row, "assignee_username"),
        row["summary"].as<std::string>(),
        nullableString(row, "result")};
}

domain::WorkOrderAttachment workOrderAttachmentFromRow(const drogon::orm::Row& row) {
    return domain::WorkOrderAttachment{
        row["attachment_code"].as<std::string>(),
        row["work_order_code"].as<std::string>(),
        row["file_name"].as<std::string>(),
        row["uri"].as<std::string>(),
        nullableString(row, "content_type"),
        static_cast<std::uint64_t>(row["size_bytes"].as<unsigned long long>()),
        nullableString(row, "uploaded_by_username")};
}
domain::RuntimeState runtimeStateFromRow(const drogon::orm::Row& row) {
    return domain::RuntimeState{
        row["asset_code"].as<std::string>(),
        row["state"].as<std::string>(),
        nullableString(row, "metric_summary"),
        row["reported_at"].as<std::string>(),
        row["severity"].as<std::string>()};
}

domain::AiInteraction aiInteractionFromRow(const drogon::orm::Row& row) {
    return domain::AiInteraction{
        row["interaction_code"].as<std::string>(),
        row["related_type"].as<std::string>(),
        row["related_id"].as<std::string>(),
        row["input"].as<std::string>(),
        row["output"].as<std::string>()};
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

MySqlAlertRepository::MySqlAlertRepository(drogon::orm::DbClientPtr client) : client_(std::move(client)) {}

domain::Alert MySqlAlertRepository::save(domain::Alert alert) {
    client_->execSqlSync(
        "INSERT INTO alerts(alert_code, asset_id, severity, state, title, acknowledged_by, assigned_to) "
        "VALUES(?, (SELECT id FROM equipment_assets WHERE asset_code = ?), ?, ?, ?, "
        "IF(? = '', NULL, (SELECT id FROM users WHERE username = ?)), "
        "IF(? = '', NULL, (SELECT id FROM users WHERE username = ?))) "
        "ON DUPLICATE KEY UPDATE asset_id = VALUES(asset_id), severity = VALUES(severity), state = VALUES(state), "
        "title = VALUES(title), acknowledged_by = VALUES(acknowledged_by), assigned_to = VALUES(assigned_to)",
        alert.id,
        alert.assetId,
        alertSeverityToString(alert.severity),
        alertStateToString(alert.state),
        alert.title,
        alert.acknowledgedBy,
        alert.acknowledgedBy,
        alert.assignedTo,
        alert.assignedTo);
    return alert;
}

std::vector<domain::Alert> MySqlAlertRepository::list() const {
    const auto result = client_->execSqlSync(
        "SELECT a.alert_code, ea.asset_code, a.severity, a.state, a.title, "
        "ack.username AS acknowledged_by_username, assignee.username AS assigned_to_username "
        "FROM alerts a "
        "LEFT JOIN equipment_assets ea ON ea.id = a.asset_id "
        "LEFT JOIN users ack ON ack.id = a.acknowledged_by "
        "LEFT JOIN users assignee ON assignee.id = a.assigned_to "
        "ORDER BY a.updated_at DESC, a.alert_code");

    std::vector<domain::Alert> alerts;
    for (const auto& row : result) {
        alerts.push_back(alertFromRow(row));
    }
    return alerts;
}

std::optional<domain::Alert> MySqlAlertRepository::findById(const std::string& id) const {
    const auto result = client_->execSqlSync(
        "SELECT a.alert_code, ea.asset_code, a.severity, a.state, a.title, "
        "ack.username AS acknowledged_by_username, assignee.username AS assigned_to_username "
        "FROM alerts a "
        "LEFT JOIN equipment_assets ea ON ea.id = a.asset_id "
        "LEFT JOIN users ack ON ack.id = a.acknowledged_by "
        "LEFT JOIN users assignee ON assignee.id = a.assigned_to "
        "WHERE a.alert_code = ?",
        id);
    if (result.empty()) {
        return std::nullopt;
    }
    return alertFromRow(result[0]);
}

domain::AlertRule MySqlAlertRepository::saveRule(domain::AlertRule rule) {
    client_->execSqlSync(
        "INSERT INTO alert_rules(rule_code, name, asset_id, min_severity, channel, target, enabled) "
        "VALUES(?, ?, IF(? = '', NULL, (SELECT id FROM equipment_assets WHERE asset_code = ?)), ?, ?, ?, ?) "
        "ON DUPLICATE KEY UPDATE name = VALUES(name), asset_id = VALUES(asset_id), min_severity = VALUES(min_severity), "
        "channel = VALUES(channel), target = VALUES(target), enabled = VALUES(enabled)",
        rule.id,
        rule.name,
        rule.assetId,
        rule.assetId,
        rule.minSeverity,
        rule.channel,
        rule.target,
        rule.enabled ? 1 : 0);
    return rule;
}

std::vector<domain::AlertRule> MySqlAlertRepository::listRules() const {
    const auto result = client_->execSqlSync(
        "SELECT r.rule_code, r.name, ea.asset_code, r.min_severity, r.channel, r.target, r.enabled "
        "FROM alert_rules r "
        "LEFT JOIN equipment_assets ea ON ea.id = r.asset_id "
        "ORDER BY r.updated_at DESC, r.rule_code");
    std::vector<domain::AlertRule> rules;
    for (const auto& row : result) {
        rules.push_back(alertRuleFromRow(row));
    }
    return rules;
}

domain::AlertNotification MySqlAlertRepository::saveNotification(domain::AlertNotification notification) {
    client_->execSqlSync(
        "INSERT INTO alert_notifications(notification_code, alert_id, rule_id, channel, target, status, message, attempt_count, last_error, delivered_at) "
        "VALUES(?, (SELECT id FROM alerts WHERE alert_code = ?), (SELECT id FROM alert_rules WHERE rule_code = ?), ?, ?, ?, ?, ?, ?, ?) "
        "ON DUPLICATE KEY UPDATE status = VALUES(status), message = VALUES(message), attempt_count = VALUES(attempt_count), last_error = VALUES(last_error), delivered_at = VALUES(delivered_at)",
        notification.id,
        notification.alertId,
        notification.ruleId,
        notification.channel,
        notification.target,
        notification.status,
        notification.message,
        notification.attemptCount,
        notification.lastError,
        notification.deliveredAt);
    return notification;
}

std::vector<domain::AlertNotification> MySqlAlertRepository::listNotifications() const {
    const auto result = client_->execSqlSync(
        "SELECT n.notification_code, a.alert_code, r.rule_code, n.channel, n.target, n.status, n.message, n.attempt_count, n.last_error, n.delivered_at "
        "FROM alert_notifications n "
        "JOIN alerts a ON a.id = n.alert_id "
        "JOIN alert_rules r ON r.id = n.rule_id "
        "ORDER BY n.created_at DESC, n.notification_code");
    std::vector<domain::AlertNotification> notifications;
    for (const auto& row : result) {
        notifications.push_back(alertNotificationFromRow(row));
    }
    return notifications;
}
MySqlWorkOrderRepository::MySqlWorkOrderRepository(drogon::orm::DbClientPtr client) : client_(std::move(client)) {}

domain::WorkOrder MySqlWorkOrderRepository::save(domain::WorkOrder order) {
    client_->execSqlSync(
        "INSERT INTO work_orders(work_order_code, asset_id, alert_id, state, summary, assignee, result) "
        "VALUES(?, (SELECT id FROM equipment_assets WHERE asset_code = ?), "
        "IF(? = '', NULL, (SELECT id FROM alerts WHERE alert_code = ?)), ?, ?, "
        "IF(? = '', NULL, (SELECT id FROM users WHERE username = ?)), ?) "
        "ON DUPLICATE KEY UPDATE asset_id = VALUES(asset_id), alert_id = VALUES(alert_id), state = VALUES(state), "
        "summary = VALUES(summary), assignee = VALUES(assignee), result = VALUES(result)",
        order.id,
        order.assetId,
        order.alertId,
        order.alertId,
        workOrderStateToString(order.state),
        order.summary,
        order.assignee,
        order.assignee,
        order.result);
    return order;
}

std::vector<domain::WorkOrder> MySqlWorkOrderRepository::list() const {
    const auto result = client_->execSqlSync(
        "SELECT wo.work_order_code, ea.asset_code, a.alert_code, wo.state, wo.summary, u.username AS assignee_username, wo.result "
        "FROM work_orders wo "
        "LEFT JOIN equipment_assets ea ON ea.id = wo.asset_id "
        "LEFT JOIN alerts a ON a.id = wo.alert_id "
        "LEFT JOIN users u ON u.id = wo.assignee "
        "ORDER BY wo.updated_at DESC, wo.work_order_code");

    std::vector<domain::WorkOrder> orders;
    for (const auto& row : result) {
        orders.push_back(workOrderFromRow(row));
    }
    return orders;
}

std::optional<domain::WorkOrder> MySqlWorkOrderRepository::findById(const std::string& id) const {
    const auto result = client_->execSqlSync(
        "SELECT wo.work_order_code, ea.asset_code, a.alert_code, wo.state, wo.summary, u.username AS assignee_username, wo.result "
        "FROM work_orders wo "
        "LEFT JOIN equipment_assets ea ON ea.id = wo.asset_id "
        "LEFT JOIN alerts a ON a.id = wo.alert_id "
        "LEFT JOIN users u ON u.id = wo.assignee "
        "WHERE wo.work_order_code = ?",
        id);
    if (result.empty()) {
        return std::nullopt;
    }
    return workOrderFromRow(result[0]);
}

std::vector<domain::WorkOrder> MySqlWorkOrderRepository::historyForAsset(const std::string& assetId) const {
    const auto result = client_->execSqlSync(
        "SELECT wo.work_order_code, ea.asset_code, a.alert_code, wo.state, wo.summary, u.username AS assignee_username, wo.result "
        "FROM work_orders wo "
        "LEFT JOIN equipment_assets ea ON ea.id = wo.asset_id "
        "LEFT JOIN alerts a ON a.id = wo.alert_id "
        "LEFT JOIN users u ON u.id = wo.assignee "
        "WHERE ea.asset_code = ? AND wo.state = 'closed' "
        "ORDER BY wo.updated_at DESC, wo.work_order_code",
        assetId);

    std::vector<domain::WorkOrder> orders;
    for (const auto& row : result) {
        orders.push_back(workOrderFromRow(row));
    }
    return orders;
}

domain::WorkOrderAttachment MySqlWorkOrderRepository::saveAttachment(domain::WorkOrderAttachment attachment) {
    client_->execSqlSync(
        "INSERT INTO work_order_attachments(attachment_code, work_order_id, file_name, uri, content_type, size_bytes, uploaded_by) "
        "VALUES(?, (SELECT id FROM work_orders WHERE work_order_code = ?), ?, ?, ?, ?, "
        "IF(? = '', NULL, (SELECT id FROM users WHERE username = ?))) "
        "ON DUPLICATE KEY UPDATE file_name = VALUES(file_name), uri = VALUES(uri), content_type = VALUES(content_type), "
        "size_bytes = VALUES(size_bytes), uploaded_by = VALUES(uploaded_by)",
        attachment.id,
        attachment.workOrderId,
        attachment.fileName,
        attachment.uri,
        attachment.contentType,
        static_cast<unsigned long long>(attachment.sizeBytes),
        attachment.uploadedBy,
        attachment.uploadedBy);
    return attachment;
}

std::vector<domain::WorkOrderAttachment> MySqlWorkOrderRepository::listAttachments(const std::string& workOrderId) const {
    const auto result = client_->execSqlSync(
        "SELECT a.attachment_code, wo.work_order_code, a.file_name, a.uri, a.content_type, a.size_bytes, u.username AS uploaded_by_username "
        "FROM work_order_attachments a "
        "JOIN work_orders wo ON wo.id = a.work_order_id "
        "LEFT JOIN users u ON u.id = a.uploaded_by "
        "WHERE wo.work_order_code = ? "
        "ORDER BY a.created_at DESC, a.attachment_code",
        workOrderId);

    std::vector<domain::WorkOrderAttachment> attachments;
    for (const auto& row : result) {
        attachments.push_back(workOrderAttachmentFromRow(row));
    }
    return attachments;
}
MySqlRuntimeStateRepository::MySqlRuntimeStateRepository(drogon::orm::DbClientPtr client) : client_(std::move(client)) {}

domain::RuntimeState MySqlRuntimeStateRepository::save(domain::RuntimeState state) {
    client_->execSqlSync(
        "INSERT INTO runtime_states(asset_code, state, metric_summary, severity, reported_at) "
        "VALUES(?, ?, ?, ?, ?) "
        "ON DUPLICATE KEY UPDATE state = VALUES(state), metric_summary = VALUES(metric_summary), "
        "severity = VALUES(severity), reported_at = VALUES(reported_at)",
        state.assetId,
        state.state,
        state.metricSummary,
        state.severity,
        state.updatedAt);
    return state;
}

std::vector<domain::RuntimeState> MySqlRuntimeStateRepository::list() const {
    const auto result = client_->execSqlSync(
        "SELECT asset_code, state, metric_summary, severity, reported_at "
        "FROM runtime_states ORDER BY updated_at DESC, asset_code");

    std::vector<domain::RuntimeState> states;
    for (const auto& row : result) {
        states.push_back(runtimeStateFromRow(row));
    }
    return states;
}

std::optional<domain::RuntimeState> MySqlRuntimeStateRepository::findByAssetId(const std::string& assetId) const {
    const auto result = client_->execSqlSync(
        "SELECT asset_code, state, metric_summary, severity, reported_at "
        "FROM runtime_states WHERE asset_code = ?",
        assetId);
    if (result.empty()) {
        return std::nullopt;
    }
    return runtimeStateFromRow(result[0]);
}

MySqlAiInteractionRepository::MySqlAiInteractionRepository(drogon::orm::DbClientPtr client) : client_(std::move(client)) {}

domain::AiInteraction MySqlAiInteractionRepository::save(domain::AiInteraction interaction) {
    client_->execSqlSync(
        "INSERT INTO ai_interactions(interaction_code, related_type, related_id, input, output) "
        "VALUES(?, ?, ?, ?, ?) "
        "ON DUPLICATE KEY UPDATE related_type = VALUES(related_type), related_id = VALUES(related_id), "
        "input = VALUES(input), output = VALUES(output)",
        interaction.id,
        interaction.relatedType,
        interaction.relatedId,
        interaction.input,
        interaction.output);
    return interaction;
}

std::vector<domain::AiInteraction> MySqlAiInteractionRepository::list() const {
    const auto result = client_->execSqlSync(
        "SELECT interaction_code, related_type, related_id, input, output "
        "FROM ai_interactions ORDER BY created_at DESC, interaction_code");

    std::vector<domain::AiInteraction> interactions;
    for (const auto& row : result) {
        interactions.push_back(aiInteractionFromRow(row));
    }
    return interactions;
}

}  // namespace induspilot::data

#endif  // INDUSPILOT_WITH_DROGON