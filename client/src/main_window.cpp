#include "main_window.hpp"

#include <QAbstractItemView>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
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
    pages_->addWidget(buildTablePage("运行监控", {"设备", "状态", "指标", "更新时间"}, api_.monitoringStates()));
    pages_->addWidget(buildTablePage("告警中心", {"级别", "设备", "标题", "状态", "负责人"}, api_.alerts()));
    pages_->addWidget(buildTablePage("维护工单", {"工单", "设备", "来源告警", "状态", "处理人"}, api_.workOrders()));
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
    auto* loginButton = new QPushButton("登录并同步资产");
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
    layout->addWidget(new QLabel("基础联机阶段：登录和资产列表可接入后端，其他模块保留离线演示数据。"));
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
    layout->addWidget(new QLabel("AI 辅助诊断"));
    auto* text = new QTextEdit(page);
    text->setText(api_.aiUnavailableMessage());
    text->setPlaceholderText("选择告警后在这里生成解释、排查建议和日志摘要。");
    layout->addWidget(text);
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

void MainWindow::refreshAssetTable() {
    if (!assetTable_) {
        return;
    }
    fillTable(assetTable_, {"编号", "名称", "类型", "产线", "状态"}, api_.assets());
    if (assetModeLabel_) {
        assetModeLabel_->setText(api_.online() ? "设备资产（后端同步）" : "设备资产（离线演示数据）");
    }
}

void MainWindow::handleLogin() {
    if (api_.login(usernameInput_->text(), passwordInput_->text())) {
        loginMessage_->setText(api_.statusMessage() + "；当前用户：" + api_.currentUser());
        refreshAssetTable();
        navigation_->setCurrentRow(1);
    } else {
        loginMessage_->setText("登录失败：" + api_.statusMessage());
    }
}
