#pragma once

#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVector>

class QNetworkAccessManager;

struct TableRow {
    QStringList columns;
};

class ApiClient {
public:
    ApiClient();

    bool login(const QString& username, const QString& password);
    QString currentUser() const;
    QString statusMessage() const;
    bool online() const;
    QVector<TableRow> assets();
    QVector<TableRow> monitoringStates() const;
    QVector<TableRow> alerts() const;
    QVector<TableRow> workOrders() const;
    QString aiUnavailableMessage() const;

private:
    QVector<TableRow> offlineAssets() const;
    QUrl endpoint(const QString& path) const;
    QByteArray requestJson(const QString& method, const QString& path, const QByteArray& body = QByteArray());
    void loadConfig();

    QString currentUser_;
    QString token_;
    QString statusMessage_{"离线演示模式"};
    QUrl apiBaseUrl_{"http://127.0.0.1:8080"};
    QNetworkAccessManager* network_{nullptr};
};