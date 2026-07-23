#include "main_window.hpp"

#include <QAbstractItemView>
#include <QComboBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QListWidget>
#include <QPushButton>
#include <QSplitter>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextEdit>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("IndusPilot 工业智能运维支持平台");
    resize(1280, 760);

    auto* splitter = new QSplitter(this);
    navigation_ = new QListWidget(splitter);
    pages_ = new QStackedWidget(splitter);

    navigation_->addItems({"登录", "总览", "设备资产", "运行监控", "告警中心", "维护工单", "AI 辅助"});
    navigation_->setFixedWidth(180);

    pages_->addWidget(buildLoginPage());
    pages_->addWidget(buildDashboardPage());
    pages_->addWidget(buildAssetPage());
    pages_->addWidget(buildMonitoringPage());
    pages_->addWidget(buildAlertPage());
    pages_->addWidget(buildWorkOrderPage());
    pages_->addWidget(buildAiPage());

    connect(navigation_, &QListWidget::currentRowChanged, pages_, &QStackedWidget::setCurrentIndex);
    navigation_->setCurrentRow(0);

    splitter->addWidget(navigation_);
    splitter->addWidget(pages_);
    setCentralWidget(splitter);
}

QWidget* MainWindow::buildLoginPage() {
    auto* page = new QWidget(this);
    auto* layout = new QFormLayout(page);
    layout->addRow(new QLabel("IndusPilot 后端登录"));
    usernameInput_ = new QLineEdit("admin");
    passwordInput_ = new QLineEdit("admin123");
    passwordInput_->setEchoMode(QLineEdit::Password);
    loginMessage_ = new QLabel(api_.statusMessage());
    auto* loginButton = new QPushButton("登录并同步列表");
    connect(loginButton, &QPushButton::clicked, this, &MainWindow::handleLogin);
    layout->addRow("账号", usernameInput_);
    layout->addRow("密码", passwordInput_);
    layout->addRow(loginButton);
    layout->addRow(loginMessage_);
    return page;
}

QWidget* MainWindow::buildDashboardPage() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->addWidget(new QLabel("设备运行总览"));
    layout->addWidget(new QLabel("基础联机阶段：登录、资产、运行监控、告警、工单和 AI 诊断可接入后端。"));
    layout->addWidget(statusBadge("critical", "danger"));
    layout->addStretch();
    return page;
}

QWidget* MainWindow::buildAssetPage() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    assetModeLabel_ = new QLabel("设备资产（离线演示数据）", page);
    layout->addWidget(assetModeLabel_);
    assetTable_ = new QTableWidget(page);
    fillTable(assetTable_, {"编号", "名称", "类型", "产线", "状态"}, api_.assets());
    layout->addWidget(assetTable_);
    return page;
}

QWidget* MainWindow::buildMonitoringPage() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    monitoringModeLabel_ = new QLabel("运行监控（离线演示数据）", page);
    layout->addWidget(monitoringModeLabel_);
    monitoringTable_ = new QTableWidget(page);
    fillTable(monitoringTable_, {"设备", "状态", "指标", "更新时间"}, api_.monitoringStates());
    layout->addWidget(monitoringTable_);
    return page;
}

QWidget* MainWindow::buildAlertPage() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    alertModeLabel_ = new QLabel("告警中心（离线演示数据）", page);
    layout->addWidget(alertModeLabel_);
    alertTable_ = new QTableWidget(page);
    fillTable(alertTable_, {"级别", "设备", "标题", "状态", "负责人"}, api_.alerts());
    layout->addWidget(alertTable_);
    return page;
}

QWidget* MainWindow::buildWorkOrderPage() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    workOrderModeLabel_ = new QLabel("维护工单（离线演示数据）", page);
    layout->addWidget(workOrderModeLabel_);

    workOrderTable_ = new QTableWidget(page);
    fillTable(workOrderTable_, {"工单", "设备", "来源告警", "状态", "处理人"}, api_.workOrders());
    layout->addWidget(workOrderTable_);

    auto* actionRow = new QWidget(page);
    auto* actionLayout = new QHBoxLayout(actionRow);
    auto* startButton = new QPushButton("开始处理", actionRow);
    auto* completeButton = new QPushButton("完成", actionRow);
    auto* closeButton = new QPushButton("关闭", actionRow);
    connect(startButton, &QPushButton::clicked, this, &MainWindow::handleStartWorkOrder);
    connect(completeButton, &QPushButton::clicked, this, &MainWindow::handleCompleteWorkOrder);
    connect(closeButton, &QPushButton::clicked, this, &MainWindow::handleCloseWorkOrder);
    actionLayout->addWidget(startButton);
    actionLayout->addWidget(completeButton);
    actionLayout->addWidget(closeButton);
    actionLayout->addStretch();
    layout->addWidget(actionRow);
    return page;
}

QWidget* MainWindow::buildTablePage(const QString& title, const QStringList& headers, const QVector<TableRow>& rows) {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->addWidget(new QLabel(title + "（离线演示数据）"));
    auto* table = new QTableWidget(page);
    fillTable(table, headers, rows);
    layout->addWidget(table);
    return page;
}

QWidget* MainWindow::buildAiPage() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    aiModeLabel_ = new QLabel("AI 辅助诊断（离线演示数据）", page);
    layout->addWidget(aiModeLabel_);

    auto* form = new QFormLayout();
    aiRelatedTypeInput_ = new QComboBox(page);
    aiRelatedTypeInput_->addItems({"alert", "work-order", "asset"});
    aiRelatedIdInput_ = new QLineEdit("alert-001", page);
    aiAssetIdInput_ = new QLineEdit("asset-001", page);
    aiAlertTitleInput_ = new QLineEdit("温度异常", page);
    aiRuntimeStateInput_ = new QLineEdit("critical", page);
    aiSeverityInput_ = new QLineEdit("critical", page);
    aiMetricSummaryInput_ = new QLineEdit("温度持续高于阈值", page);
    aiWorkOrderHistoryInput_ = new QTextEdit(page);
    aiWorkOrderHistoryInput_->setFixedHeight(72);
    aiWorkOrderHistoryInput_->setPlainText("最近一次维护更换轴承并清理散热通道");
    aiOperatorDescriptionInput_ = new QTextEdit(page);
    aiOperatorDescriptionInput_->setFixedHeight(72);
    aiOperatorDescriptionInput_->setPlainText("现场有异味，设备振动略高");
    aiPromptInput_ = new QTextEdit(page);
    aiPromptInput_->setFixedHeight(84);
    aiPromptInput_->setPlainText("诊断当前告警的可能原因，并给出现场处置建议");

    form->addRow("关联类型", aiRelatedTypeInput_);
    form->addRow("关联对象", aiRelatedIdInput_);
    form->addRow("设备编号", aiAssetIdInput_);
    form->addRow("告警标题", aiAlertTitleInput_);
    form->addRow("运行状态", aiRuntimeStateInput_);
    form->addRow("严重度", aiSeverityInput_);
    form->addRow("指标摘要", aiMetricSummaryInput_);
    form->addRow("工单历史", aiWorkOrderHistoryInput_);
    form->addRow("人工描述", aiOperatorDescriptionInput_);
    form->addRow("诊断问题", aiPromptInput_);
    layout->addLayout(form);

    auto* actionRow = new QWidget(page);
    auto* actionLayout = new QHBoxLayout(actionRow);
    auto* diagnoseButton = new QPushButton("执行 AI 诊断", actionRow);
    auto* refreshHistoryButton = new QPushButton("刷新交互审计", actionRow);
    connect(diagnoseButton, &QPushButton::clicked, this, &MainWindow::handleAiDiagnosis);
    connect(refreshHistoryButton, &QPushButton::clicked, this, &MainWindow::handleRefreshAiInteractions);
    actionLayout->addWidget(diagnoseButton);
    actionLayout->addWidget(refreshHistoryButton);
    actionLayout->addStretch();
    layout->addWidget(actionRow);

    aiResultOutput_ = new QTextEdit(page);
    aiResultOutput_->setReadOnly(true);
    aiResultOutput_->setText(api_.aiUnavailableMessage());
    layout->addWidget(aiResultOutput_);

    layout->addWidget(new QLabel("AI 交互审计", page));
    aiInteractionTable_ = new QTableWidget(page);
    fillTable(aiInteractionTable_, {"编号", "关联类型", "关联对象", "输入", "输出"}, api_.aiInteractions());
    layout->addWidget(aiInteractionTable_);
    return page;
}

QLabel* MainWindow::statusBadge(const QString& text, const QString& tone) {
    auto* label = new QLabel(text, this);
    const auto color = tone == "danger" ? "#b42318" : "#175cd3";
    label->setStyleSheet(QString("QLabel { color: white; background: %1; padding: 4px 8px; border-radius: 4px; }").arg(color));
    return label;
}

void MainWindow::fillTable(QTableWidget* table, const QStringList& headers, const QVector<TableRow>& rows) {
    table->clear();
    table->setRowCount(rows.size());
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    for (int row = 0; row < rows.size(); ++row) {
        for (int column = 0; column < rows[row].columns.size() && column < headers.size(); ++column) {
            table->setItem(row, column, new QTableWidgetItem(rows[row].columns[column]));
        }
    }
    table->horizontalHeader()->setStretchLastSection(true);
    table->setAlternatingRowColors(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void MainWindow::refreshOnlineTables() {
    if (assetTable_) {
        fillTable(assetTable_, {"编号", "名称", "类型", "产线", "状态"}, api_.assets());
    }
    if (monitoringTable_) {
        fillTable(monitoringTable_, {"设备", "状态", "指标", "更新时间"}, api_.monitoringStates());
    }
    if (alertTable_) {
        fillTable(alertTable_, {"级别", "设备", "标题", "状态", "负责人"}, api_.alerts());
    }
    refreshWorkOrderTable();
    refreshAiInteractionTable();

    const auto modeText = api_.online() ? "后端同步" : "离线演示数据";
    if (assetModeLabel_) {
        assetModeLabel_->setText(QStringLiteral("设备资产（") + modeText + QStringLiteral("）"));
    }
    if (monitoringModeLabel_) {
        monitoringModeLabel_->setText(QStringLiteral("运行监控（") + modeText + QStringLiteral("）"));
    }
    if (alertModeLabel_) {
        alertModeLabel_->setText(QStringLiteral("告警中心（") + modeText + QStringLiteral("）"));
    }
    if (workOrderModeLabel_) {
        workOrderModeLabel_->setText(QStringLiteral("维护工单（") + modeText + QStringLiteral("）"));
    }
    if (aiModeLabel_) {
        aiModeLabel_->setText(QStringLiteral("AI 辅助诊断（") + modeText + QStringLiteral("）"));
    }
}

void MainWindow::refreshWorkOrderTable() {
    if (workOrderTable_) {
        fillTable(workOrderTable_, {"工单", "设备", "来源告警", "状态", "处理人"}, api_.workOrders());
    }
}

void MainWindow::refreshAiInteractionTable() {
    if (aiInteractionTable_) {
        const auto relatedType = aiRelatedTypeInput_ ? aiRelatedTypeInput_->currentText() : QString{};
        const auto relatedId = aiRelatedIdInput_ ? aiRelatedIdInput_->text() : QString{};
        fillTable(aiInteractionTable_, {"编号", "关联类型", "关联对象", "输入", "输出"}, api_.aiInteractions(relatedType, relatedId));
    }
}

QString MainWindow::selectedWorkOrderId() const {
    if (!workOrderTable_ || workOrderTable_->currentRow() < 0) {
        return {};
    }
    const auto* item = workOrderTable_->item(workOrderTable_->currentRow(), 0);
    return item ? item->text() : QString{};
}

void MainWindow::handleLogin() {
    if (api_.login(usernameInput_->text(), passwordInput_->text())) {
        loginMessage_->setText(api_.statusMessage() + "；当前用户：" + api_.currentUser());
        refreshOnlineTables();
        navigation_->setCurrentRow(1);
    } else {
        loginMessage_->setText("登录失败：" + api_.statusMessage());
    }
}

void MainWindow::handleStartWorkOrder() {
    const auto orderId = selectedWorkOrderId();
    if (api_.startWorkOrder(orderId)) {
        refreshWorkOrderTable();
    }
    QMessageBox::information(this, "工单操作", api_.statusMessage());
}

void MainWindow::handleCompleteWorkOrder() {
    const auto orderId = selectedWorkOrderId();
    if (orderId.isEmpty()) {
        api_.completeWorkOrder(orderId, QString{});
        QMessageBox::information(this, "工单操作", api_.statusMessage());
        return;
    }
    bool ok = false;
    const auto result = QInputDialog::getText(this, "完成工单", "处理结果", QLineEdit::Normal, "已完成现场处理", &ok);
    if (!ok) {
        return;
    }
    if (api_.completeWorkOrder(orderId, result)) {
        refreshWorkOrderTable();
    }
    QMessageBox::information(this, "工单操作", api_.statusMessage());
}

void MainWindow::handleCloseWorkOrder() {
    const auto orderId = selectedWorkOrderId();
    if (api_.closeWorkOrder(orderId)) {
        refreshWorkOrderTable();
    }
    QMessageBox::information(this, "工单操作", api_.statusMessage());
}

void MainWindow::handleAiDiagnosis() {
    AiDiagnosisInput input;
    input.relatedType = aiRelatedTypeInput_ ? aiRelatedTypeInput_->currentText() : QStringLiteral("alert");
    input.relatedId = aiRelatedIdInput_ ? aiRelatedIdInput_->text() : QString{};
    input.assetId = aiAssetIdInput_ ? aiAssetIdInput_->text() : QString{};
    input.alertTitle = aiAlertTitleInput_ ? aiAlertTitleInput_->text() : QString{};
    input.runtimeState = aiRuntimeStateInput_ ? aiRuntimeStateInput_->text() : QString{};
    input.severity = aiSeverityInput_ ? aiSeverityInput_->text() : QString{};
    input.metricSummary = aiMetricSummaryInput_ ? aiMetricSummaryInput_->text() : QString{};
    input.workOrderHistory = aiWorkOrderHistoryInput_ ? aiWorkOrderHistoryInput_->toPlainText() : QString{};
    input.operatorDescription = aiOperatorDescriptionInput_ ? aiOperatorDescriptionInput_->toPlainText() : QString{};
    input.prompt = aiPromptInput_ ? aiPromptInput_->toPlainText() : QString{};

    const auto report = api_.diagnose(input);
    if (aiResultOutput_) {
        aiResultOutput_->setText(report.isEmpty() ? api_.statusMessage() : report);
    }
    refreshAiInteractionTable();
    if (aiModeLabel_) {
        const auto modeText = api_.online() ? "后端同步" : "离线演示数据";
        aiModeLabel_->setText(QStringLiteral("AI 辅助诊断（") + modeText + QStringLiteral("）"));
    }
}

void MainWindow::handleRefreshAiInteractions() {
    refreshAiInteractionTable();
    if (aiResultOutput_ && !api_.statusMessage().isEmpty()) {
        aiResultOutput_->setText(api_.statusMessage());
    }
}
