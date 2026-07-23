#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace induspilot::domain {

enum class AssetStatus { Active, Inactive, Maintenance, Retired };
enum class AlertSeverity { Info, Warning, Critical };
enum class AlertState { Open, Acknowledged, Assigned, Resolved, Closed };
enum class WorkOrderState { Created, Assigned, Processing, Completed, Closed };

struct User {
    std::string id;
    std::string username;
    std::vector<std::string> roles;
};

struct EquipmentAsset {
    std::string id;
    std::string name;
    std::string type;
    std::string factory;
    std::string workshop;
    std::string productionLine;
    AssetStatus status{AssetStatus::Active};
};

struct Alert {
    std::string id;
    std::string assetId;
    AlertSeverity severity{AlertSeverity::Info};
    AlertState state{AlertState::Open};
    std::string title;
    std::string acknowledgedBy;
    std::string assignedTo;
};

struct AlertRule {
    std::string id;
    std::string name;
    std::string assetId;
    std::string minSeverity{"warning"};
    std::string channel{"console"};
    std::string target;
    bool enabled{true};
};

struct AlertNotification {
    std::string id;
    std::string alertId;
    std::string ruleId;
    std::string channel;
    std::string target;
    std::string status{"queued"};
    std::string message;
    int attemptCount{0};
    std::string lastError;
    std::string deliveredAt;
};

struct WorkOrderAttachment {
    std::string id;
    std::string workOrderId;
    std::string fileName;
    std::string uri;
    std::string contentType;
    std::uint64_t sizeBytes{0};
    std::string uploadedBy;
};

struct WorkOrder {
    std::string id;
    std::string assetId;
    std::string alertId;
    WorkOrderState state{WorkOrderState::Created};
    std::string assignee;
    std::string summary;
    std::string result;
};

struct RuntimeState {
    std::string assetId;
    std::string state{"unknown"};
    std::string metricSummary;
    std::string updatedAt;
    std::string severity{"info"};
};

struct OperationAuditEvent {
    std::string id;
    std::string actor;
    std::string action;
    std::string resourceType;
    std::string resourceId;
    std::string result{"success"};
    std::string traceId;
    std::string occurredAt;
};
struct AiInteraction {
    std::string id;
    std::string relatedType;
    std::string relatedId;
    std::string input;
    std::string output;
};

}  // namespace induspilot::domain