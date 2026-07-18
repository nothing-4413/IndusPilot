#include "main_window.hpp"

#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSplitter>
#include <QStackedWidget>
#include <QStringList>
#include <QTableWidget>
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
    pages_->addWidget(buildTablePage("设备资产", {"编号", "名称", "类型", "产线", "状态"}));
    pages_->addWidget(buildTablePage("运行监控", {"设备", "状态", "指标", "更新时间"}));
    pages_->addWidget(buildTablePage("告警中心", {"级别", "设备", "标题", "状态", "负责人"}));
    pages_->addWidget(buildTablePage("维护工单", {"工单", "设备", "来源告警", "状态", "处理人"}));
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
    layout->addRow(new QLabel("IndusPilot 登录"));
    layout->addRow("账号", new QLineEdit("admin"));
    auto* password = new QLineEdit;
    password->setEchoMode(QLineEdit::Password);
    password->setText("admin123");
    layout->addRow("密码", password);
    layout->addRow(new QPushButton("进入平台"));
    return page;
}

QWidget* MainWindow::buildDashboardPage() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->addWidget(new QLabel("设备运行总览"));
    layout->addWidget(new QLabel("在线设备：0    活跃告警：0    待处理工单：0    AI 建议：未启用"));
    layout->addStretch();
    return page;
}

QWidget* MainWindow::buildTablePage(const QString& title, const QStringList& headers) {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->addWidget(new QLabel(title));
    auto* table = new QTableWidget(0, headers.size(), page);
    table->setHorizontalHeaderLabels(headers);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setAlternatingRowColors(true);
    layout->addWidget(table);
    return page;
}

QWidget* MainWindow::buildAiPage() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->addWidget(new QLabel("AI 辅助诊断"));
    auto* text = new QTextEdit(page);
    text->setPlaceholderText("选择告警后在这里生成解释、排查建议和日志摘要。");
    layout->addWidget(text);
    return page;
}