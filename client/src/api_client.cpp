#include "api_client.hpp"

bool ApiClient::login(const QString& username, const QString& password) {
    if (username.isEmpty() || password.isEmpty()) {
        return false;
    }
    currentUser_ = username;
    return true;
}

QString ApiClient::currentUser() const {
    return currentUser_;
}

QVector<TableRow> ApiClient::assets() const {
    return {{{"asset-001", "一号产线主电机", "motor", "一号产线", "维护中"}}};
}

QVector<TableRow> ApiClient::monitoringStates() const {
    return {{{"一号产线主电机", "warning", "温度偏高", "now"}}};
}

QVector<TableRow> ApiClient::alerts() const {
    return {{{"critical", "一号产线主电机", "温度异常", "已分派", "maintainer"}}};
}

QVector<TableRow> ApiClient::workOrders() const {
    return {{{"wo-from-alert-001", "一号产线主电机", "alert-001", "处理中", "maintainer"}}};
}

QString ApiClient::aiUnavailableMessage() const {
    return "AI 服务未启用：当前仅展示辅助诊断入口，核心告警和工单流程不受影响。";
}