#pragma once

#include "api_client.hpp"

#include <QMainWindow>
#include <QString>
#include <QStringList>
#include <QVector>

class QLabel;
class QLineEdit;
class QListWidget;
class QStackedWidget;
class QTableWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    QWidget* buildLoginPage();
    QWidget* buildDashboardPage();
    QWidget* buildAssetPage();
    QWidget* buildTablePage(const QString& title, const QStringList& headers, const QVector<TableRow>& rows);
    QWidget* buildAiPage();
    QLabel* statusBadge(const QString& text, const QString& tone);
    void fillTable(QTableWidget* table, const QStringList& headers, const QVector<TableRow>& rows);
    void refreshAssetTable();
    void handleLogin();

    ApiClient api_;
    QLineEdit* usernameInput_{nullptr};
    QLineEdit* passwordInput_{nullptr};
    QLabel* loginMessage_{nullptr};
    QLabel* assetModeLabel_{nullptr};
    QListWidget* navigation_{nullptr};
    QStackedWidget* pages_{nullptr};
    QTableWidget* assetTable_{nullptr};
};