#pragma once

#include <QString>
#include <QStringList>
#include <QUrl>
#include <QJsonValue>
#include <QJsonObject>
#include <QVector>

class QNetworkAccessManager;

struct TableRow {
    QStringList columns;
};

struct AiDiagnosisInput {
    QString relatedType{"alert"};
    QString relatedId;
    QString prompt;
    QString assetId;
    QString alertTitle;
    QString runtimeState;
    QString severity;
    QString metricSummary;
    QString workOrderHistory;
    QString operatorDescription;
    QStringList contextItems;
};

class ApiClient {
public:
    ApiClient();

    bool login(const QString& username, const QString& password);
    QString currentUser() const;
    QString statusMessage() const;
    bool online() const;
    QVector<TableRow> assets();
    QVector<TableRow> monitoringStates();
    QVector<TableRow> alerts();
    QVector<TableRow> workOrders();
    bool createWorkOrder(const QString& orderId, const QString& assetId, const QString& alertId, const QString& summary, const QString& assignee);
    bool assignWorkOrder(const QString& orderId, const QString& assignee);
    bool startWorkOrder(const QString& orderId);
    bool completeWorkOrder(const QString& orderId, const QString& result);
    bool closeWorkOrder(const QString& orderId);
    QString diagnose(const AiDiagnosisInput& input);
    QVector<TableRow> aiInteractions(const QString& relatedType = QString(), const QString& relatedId = QString());
    QString aiUnavailableMessage() const;

private:
    QVector<TableRow> offlineAssets() const;
    QVector<TableRow> offlineMonitoringStates() const;
    QVector<TableRow> offlineAlerts() const;
    QVector<TableRow> offlineWorkOrders() const;
    QVector<TableRow> offlineAiInteractions() const;
    QString offlineAiDiagnosis(const AiDiagnosisInput& input) const;
    QJsonObject responseEnvelope(const QString& path, QJsonValue::Type dataType);
    QJsonObject postEnvelope(
        const QString& path,
        const QJsonObject& payload,
        QJsonValue::Type dataType,
        const QString& successMessage = QString(),
        const QString& failureMessage = QString());
    QUrl endpoint(const QString& path) const;
    QByteArray requestJson(const QString& method, const QString& path, const QByteArray& body = QByteArray());
    void loadConfig();

    QString currentUser_;
    QString token_;
    QString statusMessage_{"离线演示模式"};
    QUrl apiBaseUrl_{"http://127.0.0.1:8080"};
    QNetworkAccessManager* network_{nullptr};
};
