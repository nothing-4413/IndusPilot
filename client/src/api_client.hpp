#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

struct TableRow {
    QStringList columns;
};

class ApiClient {
public:
    bool login(const QString& username, const QString& password);
    QString currentUser() const;
    QVector<TableRow> assets() const;
    QVector<TableRow> monitoringStates() const;
    QVector<TableRow> alerts() const;
    QVector<TableRow> workOrders() const;
    QString aiUnavailableMessage() const;

private:
    QString currentUser_;
};