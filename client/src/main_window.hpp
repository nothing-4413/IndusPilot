#pragma once

#include "api_client.hpp"

#include <QMainWindow>
#include <QString>
#include <QStringList>
#include <QVector>

class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QSpinBox;
class QStackedWidget;
class QTableWidget;
class QTextEdit;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    QWidget* buildLoginPage();
    QWidget* buildDashboardPage();
    QWidget* buildAssetPage();
    QWidget* buildMonitoringPage();
    QWidget* buildAlertPage();
    QWidget* buildWorkOrderPage();
    QWidget* buildTablePage(const QString& title, const QStringList& headers, const QVector<TableRow>& rows);
    QWidget* buildAiPage();
    QLabel* statusBadge(const QString& text, const QString& tone);
    void fillTable(QTableWidget* table, const QStringList& headers, const QVector<TableRow>& rows);
    void refreshOnlineTables();
    void refreshAssetTable();
    void refreshMonitoringTable();
    void refreshAlertTable();
    void refreshWorkOrderTable();
    void refreshAiInteractionTable();
    QString selectedAssetId() const;
    QString selectedAlertId() const;
    QString selectedWorkOrderId() const;
    void handleLogin();
    void handleUpdateAssetStatus();
    void handleSubmitMonitoringState();
    void handleCreateAlert();
    void handleAcknowledgeAlert();
    void handleAssignAlert();
    void handleResolveAlert();
    void handleCloseAlert();
    void handleCreateWorkOrderFromAlert();
    void handleCreateWorkOrder();
    void handleAssignWorkOrder();
    void handleStartWorkOrder();
    void handleCompleteWorkOrder();
    void handleCloseWorkOrder();
    void handleAiDiagnosis();
    void handleRefreshAiInteractions();
    void handlePreviousAiAuditPage();
    void handleNextAiAuditPage();
    void handleAiAuditLimitChanged(int value);
    void handleExportAiInteractions();

    ApiClient api_;
    QLineEdit* usernameInput_{nullptr};
    QLineEdit* passwordInput_{nullptr};
    QLabel* loginMessage_{nullptr};
    QLabel* assetModeLabel_{nullptr};
    QLabel* monitoringModeLabel_{nullptr};
    QLabel* alertModeLabel_{nullptr};
    QLabel* workOrderModeLabel_{nullptr};
    QLabel* aiModeLabel_{nullptr};
    QListWidget* navigation_{nullptr};
    QStackedWidget* pages_{nullptr};
    QTableWidget* assetTable_{nullptr};
    QTableWidget* monitoringTable_{nullptr};
    QLineEdit* monitoringAssetIdInput_{nullptr};
    QComboBox* monitoringStateInput_{nullptr};
    QComboBox* monitoringSeverityInput_{nullptr};
    QLineEdit* monitoringMetricSummaryInput_{nullptr};
    QTableWidget* alertTable_{nullptr};
    QTableWidget* workOrderTable_{nullptr};
    QComboBox* aiRelatedTypeInput_{nullptr};
    QLineEdit* aiRelatedIdInput_{nullptr};
    QLineEdit* aiAssetIdInput_{nullptr};
    QLineEdit* aiAlertTitleInput_{nullptr};
    QLineEdit* aiRuntimeStateInput_{nullptr};
    QLineEdit* aiSeverityInput_{nullptr};
    QLineEdit* aiMetricSummaryInput_{nullptr};
    QTextEdit* aiWorkOrderHistoryInput_{nullptr};
    QTextEdit* aiOperatorDescriptionInput_{nullptr};
    QTextEdit* aiPromptInput_{nullptr};
    QTextEdit* aiResultOutput_{nullptr};
    QSpinBox* aiAuditLimitInput_{nullptr};
    QLabel* aiAuditPageLabel_{nullptr};
    QPushButton* aiAuditPrevButton_{nullptr};
    QPushButton* aiAuditNextButton_{nullptr};
    int aiAuditOffset_{0};
    int aiAuditTotal_{0};
    QTableWidget* aiInteractionTable_{nullptr};
};
