#include "api_client.hpp"

#include <QCoreApplication>
#include <QEventLoop>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

namespace {

QString stringValue(const QJsonObject& object, const QString& key) {
    return object.value(key).toString();
}

QString configPath() {
    const auto appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates{
        appDir + "/config/client.json",
        appDir + "/config/client.example.json",
        QCoreApplication::applicationDirPath() + "/../config/client.json",
        QCoreApplication::applicationDirPath() + "/../config/client.example.json",
        QCoreApplication::applicationDirPath() + "/../../../config/client.json",
        QCoreApplication::applicationDirPath() + "/../../../config/client.example.json",
        "config/client.json",
        "config/client.example.json"};
    for (const auto& candidate : candidates) {
        if (QFile::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

}  // namespace

ApiClient::ApiClient() : network_(new QNetworkAccessManager(QCoreApplication::instance())) {
    loadConfig();
}

bool ApiClient::login(const QString& username, const QString& password) {
    if (username.isEmpty() || password.isEmpty()) {
        statusMessage_ = "请输入账号和密码";
        return false;
    }

    QJsonObject payload;
    payload["username"] = username;
    payload["password"] = password;
    const auto response = requestJson("POST", "/api/v1/auth/login", QJsonDocument(payload).toJson(QJsonDocument::Compact));
    if (response.isEmpty()) {
        currentUser_ = username;
        token_.clear();
        if (statusMessage_.isEmpty()) {
            statusMessage_ = "后端不可用，已进入离线演示模式";
        }
        return true;
    }

    const auto document = QJsonDocument::fromJson(response);
    const auto envelope = document.object();
    const auto data = envelope.value("data").toObject();
    const auto token = data.value("token").toString();
    if (envelope.value("success").toBool(false) && !token.isEmpty()) {
        token_ = token;
        currentUser_ = data.value("user").toObject().value("username").toString(username);
        statusMessage_ = "已连接后端：" + apiBaseUrl_.toString();
        return true;
    }

    token_.clear();
    statusMessage_ = envelope.value("message").toString("用户名或密码错误");
    return false;
}

QString ApiClient::currentUser() const {
    return currentUser_;
}

QString ApiClient::statusMessage() const {
    return statusMessage_;
}

bool ApiClient::online() const {
    return !token_.isEmpty();
}

QVector<TableRow> ApiClient::assets() {
    if (token_.isEmpty()) {
        return offlineAssets();
    }

    const auto envelope = responseEnvelope("/api/v1/assets", QJsonValue::Array);
    if (envelope.isEmpty()) {
        statusMessage_ = "资产列表同步失败，已显示离线演示数据";
        return offlineAssets();
    }

    QVector<TableRow> rows;
    const auto assets = envelope.value("data").toArray();
    for (const auto& value : assets) {
        const auto asset = value.toObject();
        rows.push_back(TableRow{{
            stringValue(asset, "id"),
            stringValue(asset, "name"),
            stringValue(asset, "type"),
            stringValue(asset, "productionLine"),
            stringValue(asset, "status")}});
    }
    if (rows.isEmpty()) {
        statusMessage_ = "后端资产列表为空";
    }
    return rows;
}

QVector<TableRow> ApiClient::monitoringStates() {
    if (token_.isEmpty()) {
        return offlineMonitoringStates();
    }

    const auto envelope = responseEnvelope("/api/v1/monitoring/states", QJsonValue::Object);
    const auto data = envelope.value("data").toObject();
    if (envelope.isEmpty() || !data.value("items").isArray()) {
        statusMessage_ = "运行监控同步失败，已显示离线演示数据";
        return offlineMonitoringStates();
    }

    QVector<TableRow> rows;
    const auto states = data.value("items").toArray();
    for (const auto& value : states) {
        const auto state = value.toObject();
        rows.push_back(TableRow{{
            stringValue(state, "assetId"),
            stringValue(state, "state"),
            stringValue(state, "metricSummary"),
            stringValue(state, "updatedAt")}});
    }
    return rows;
}

QVector<TableRow> ApiClient::alerts() {
    if (token_.isEmpty()) {
        return offlineAlerts();
    }

    const auto envelope = responseEnvelope("/api/v1/alerts", QJsonValue::Array);
    if (envelope.isEmpty()) {
        statusMessage_ = "告警列表同步失败，已显示离线演示数据";
        return offlineAlerts();
    }

    QVector<TableRow> rows;
    const auto alerts = envelope.value("data").toArray();
    for (const auto& value : alerts) {
        const auto alert = value.toObject();
        const auto assignee = stringValue(alert, "assignedTo").isEmpty() ? stringValue(alert, "acknowledgedBy") : stringValue(alert, "assignedTo");
        rows.push_back(TableRow{{
            stringValue(alert, "severity"),
            stringValue(alert, "assetId"),
            stringValue(alert, "title"),
            stringValue(alert, "state"),
            assignee}});
    }
    return rows;
}

QVector<TableRow> ApiClient::workOrders() const {
    return {{{"wo-from-alert-001", "一号产线主电机", "alert-001", "处理中", "maintainer"}}};
}

QString ApiClient::aiUnavailableMessage() const {
    return "当前客户端已接入登录、资产、运行监控和告警列表；AI 诊断页仍使用离线演示入口，核心告警和工单流程不受影响。";
}

QVector<TableRow> ApiClient::offlineAssets() const {
    return {{{"asset-001", "一号产线主电机", "motor", "一号产线", "维护中"}}};
}

QVector<TableRow> ApiClient::offlineMonitoringStates() const {
    return {{{"asset-001", "warning", "温度偏高", "now"}}};
}

QVector<TableRow> ApiClient::offlineAlerts() const {
    return {{{"critical", "asset-001", "温度异常", "已分派", "maintainer"}}};
}

QJsonObject ApiClient::responseEnvelope(const QString& path, QJsonValue::Type dataType) {
    const auto response = requestJson("GET", path);
    if (response.isEmpty()) {
        return {};
    }

    const auto envelope = QJsonDocument::fromJson(response).object();
    const auto data = envelope.value("data");
    if (!envelope.value("success").toBool(false) || data.type() != dataType) {
        if (envelope.value("code").toString().startsWith("AUTH")) {
            token_.clear();
        }
        return {};
    }
    return envelope;
}

QUrl ApiClient::endpoint(const QString& path) const {
    auto url = apiBaseUrl_;
    auto basePath = url.path();
    if (basePath.endsWith('/')) {
        basePath.chop(1);
    }
    url.setPath(basePath + path);
    return url;
}

QByteArray ApiClient::requestJson(const QString& method, const QString& path, const QByteArray& body) {
    QNetworkRequest request(endpoint(path));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!token_.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + token_.toUtf8());
    }

    QNetworkReply* reply = nullptr;
    if (method == "POST") {
        reply = network_->post(request, body);
    } else {
        reply = network_->get(request);
    }

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(5000);
    loop.exec();

    QByteArray payload;
    if (timer.isActive() && reply->error() == QNetworkReply::NoError) {
        payload = reply->readAll();
    } else if (!timer.isActive()) {
        statusMessage_ = "后端请求超时，已使用离线演示数据";
        reply->abort();
    } else {
        statusMessage_ = "后端请求失败：" + reply->errorString();
    }
    reply->deleteLater();
    return payload;
}

void ApiClient::loadConfig() {
    const auto path = configPath();
    if (path.isEmpty()) {
        statusMessage_ = "未找到客户端配置，使用默认后端地址";
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        statusMessage_ = "客户端配置读取失败，使用默认后端地址";
        return;
    }
    const auto document = QJsonDocument::fromJson(file.readAll());
    const auto url = document.object().value("apiBaseUrl").toString();
    if (!url.isEmpty()) {
        apiBaseUrl_ = QUrl(url);
    }
}
