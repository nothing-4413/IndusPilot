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
#include <QUrlQuery>

namespace {

QString stringValue(const QJsonObject& object, const QString& key) {
    return object.value(key).toString();
}

QString workOrderActionPath(const QString& orderId, const QString& action) {
    return "/api/v1/work-orders/" + QString::fromUtf8(QUrl::toPercentEncoding(orderId)) + "/" + action;
}

QString alertActionPath(const QString& alertId, const QString& action) {
    return "/api/v1/alerts/" + QString::fromUtf8(QUrl::toPercentEncoding(alertId)) + "/" + action;
}
QStringList jsonStringArray(const QJsonArray& array) {
    QStringList result;
    for (const auto& value : array) {
        const auto item = value.toString().trimmed();
        if (!item.isEmpty()) {
            result.push_back(item);
        }
    }
    return result;
}

void appendContextItem(QStringList& items, const QString& label, const QString& value) {
    const auto normalized = value.trimmed();
    if (!normalized.isEmpty()) {
        items.push_back(label + "：" + normalized);
    }
}

QString bulletSection(const QString& title, const QStringList& items) {
    QString section = title + "\n";
    if (items.isEmpty()) {
        return section + "- 暂无\n";
    }
    for (const auto& item : items) {
        section += "- " + item + "\n";
    }
    return section;
}

QString diagnosisReport(const QJsonObject& data) {
    const auto reviewText = data.value("requiresHumanReview").toBool(true) ? "需要人工复核" : "可按低风险建议执行";
    QString report;
    report += "AI 结构化诊断\n\n";
    report += QStringLiteral("摘要：") + stringValue(data, "summary") + QStringLiteral("\n");
    report += QStringLiteral("风险级别：") + stringValue(data, "riskLevel") + QStringLiteral("\n");
    report += QStringLiteral("Provider：") + stringValue(data, "provider") + QStringLiteral("\n");
    report += QStringLiteral("可用状态：") + QString(data.value("available").toBool(false) ? "在线" : "降级") + QStringLiteral("\n");
    report += QStringLiteral("人审要求：") + reviewText + QStringLiteral("\n\n");
    report += bulletSection("可能原因", jsonStringArray(data.value("possibleCauses").toArray()));
    report += "\n" + bulletSection("建议动作", jsonStringArray(data.value("recommendedActions").toArray()));
    const auto rawOutput = stringValue(data, "rawProviderOutput").trimmed();
    if (!rawOutput.isEmpty()) {
        report += QStringLiteral("\nProvider 输出：\n") + rawOutput + QStringLiteral("\n");
    }
    return report.trimmed();
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
            stringValue(alert, "id"),
            stringValue(alert, "severity"),
            stringValue(alert, "assetId"),
            stringValue(alert, "title"),
            stringValue(alert, "state"),
            assignee}});
    }
    return rows;
}


bool ApiClient::acknowledgeAlert(const QString& alertId) {
    if (token_.isEmpty() || alertId.isEmpty()) {
        statusMessage_ = "请先连接后端并选择告警";
        return false;
    }
    return !postEnvelope(alertActionPath(alertId, "acknowledge"), QJsonObject{}, QJsonValue::Object, "告警确认成功", "告警确认失败").isEmpty();
}

bool ApiClient::assignAlert(const QString& alertId, const QString& assignee) {
    const auto normalizedAssignee = assignee.trimmed();
    if (token_.isEmpty() || alertId.isEmpty()) {
        statusMessage_ = "请先连接后端并选择告警";
        return false;
    }
    if (normalizedAssignee.isEmpty()) {
        statusMessage_ = "分派告警需要填写负责人";
        return false;
    }

    QJsonObject payload;
    payload["assignee"] = normalizedAssignee;
    return !postEnvelope(alertActionPath(alertId, "assign"), payload, QJsonValue::Object, "告警分派成功", "告警分派失败").isEmpty();
}

bool ApiClient::resolveAlert(const QString& alertId) {
    if (token_.isEmpty() || alertId.isEmpty()) {
        statusMessage_ = "请先连接后端并选择告警";
        return false;
    }
    return !postEnvelope(alertActionPath(alertId, "resolve"), QJsonObject{}, QJsonValue::Object, "告警解决成功", "告警解决失败").isEmpty();
}

bool ApiClient::closeAlert(const QString& alertId) {
    if (token_.isEmpty() || alertId.isEmpty()) {
        statusMessage_ = "请先连接后端并选择告警";
        return false;
    }
    return !postEnvelope(alertActionPath(alertId, "close"), QJsonObject{}, QJsonValue::Object, "告警关闭成功", "告警关闭失败").isEmpty();
}
QVector<TableRow> ApiClient::workOrders() {
    if (token_.isEmpty()) {
        return offlineWorkOrders();
    }

    const auto envelope = responseEnvelope("/api/v1/work-orders", QJsonValue::Array);
    if (envelope.isEmpty()) {
        statusMessage_ = "工单列表同步失败，已显示离线演示数据";
        return offlineWorkOrders();
    }

    QVector<TableRow> rows;
    const auto orders = envelope.value("data").toArray();
    for (const auto& value : orders) {
        const auto order = value.toObject();
        rows.push_back(TableRow{{
            stringValue(order, "id"),
            stringValue(order, "assetId"),
            stringValue(order, "alertId"),
            stringValue(order, "state"),
            stringValue(order, "assignee"),
            stringValue(order, "summary")}});
    }
    return rows;
}


bool ApiClient::createWorkOrder(const QString& orderId, const QString& assetId, const QString& alertId, const QString& summary, const QString& assignee) {
    const auto normalizedId = orderId.trimmed();
    const auto normalizedAssetId = assetId.trimmed();
    const auto normalizedSummary = summary.trimmed();
    if (token_.isEmpty()) {
        statusMessage_ = "请先连接后端再创建工单";
        return false;
    }
    if (normalizedId.isEmpty() || normalizedAssetId.isEmpty() || normalizedSummary.isEmpty()) {
        statusMessage_ = "创建工单需要填写工单编号、设备编号和摘要";
        return false;
    }

    QJsonObject payload;
    payload["id"] = normalizedId;
    payload["assetId"] = normalizedAssetId;
    payload["summary"] = normalizedSummary;
    const auto normalizedAlertId = alertId.trimmed();
    if (!normalizedAlertId.isEmpty()) {
        payload["alertId"] = normalizedAlertId;
    }
    const auto normalizedAssignee = assignee.trimmed();
    if (!normalizedAssignee.isEmpty()) {
        payload["assignee"] = normalizedAssignee;
    }
    return !postEnvelope("/api/v1/work-orders", payload, QJsonValue::Object, "工单创建成功", "工单创建失败").isEmpty();
}

bool ApiClient::assignWorkOrder(const QString& orderId, const QString& assignee) {
    const auto normalizedAssignee = assignee.trimmed();
    if (token_.isEmpty() || orderId.isEmpty()) {
        statusMessage_ = "请先连接后端并选择工单";
        return false;
    }
    if (normalizedAssignee.isEmpty()) {
        statusMessage_ = "分派工单需要填写处理人";
        return false;
    }

    QJsonObject payload;
    payload["assignee"] = normalizedAssignee;
    return !postEnvelope(workOrderActionPath(orderId, "assign"), payload, QJsonValue::Object, "工单分派成功", "工单分派失败").isEmpty();
}
bool ApiClient::startWorkOrder(const QString& orderId) {
    if (token_.isEmpty() || orderId.isEmpty()) {
        statusMessage_ = "请先连接后端并选择工单";
        return false;
    }
    return !postEnvelope(workOrderActionPath(orderId, "start"), QJsonObject{}, QJsonValue::Object, "工单操作成功", "工单操作失败").isEmpty();
}

bool ApiClient::completeWorkOrder(const QString& orderId, const QString& result) {
    const auto normalizedResult = result.trimmed();
    if (token_.isEmpty() || orderId.isEmpty() || normalizedResult.isEmpty()) {
        statusMessage_ = "完成工单需要选择工单并填写处理结果";
        return false;
    }
    QJsonObject payload;
    payload["result"] = normalizedResult;
    return !postEnvelope(workOrderActionPath(orderId, "complete"), payload, QJsonValue::Object, "工单操作成功", "工单操作失败").isEmpty();
}

bool ApiClient::closeWorkOrder(const QString& orderId) {
    if (token_.isEmpty() || orderId.isEmpty()) {
        statusMessage_ = "请先连接后端并选择工单";
        return false;
    }
    return !postEnvelope(workOrderActionPath(orderId, "close"), QJsonObject{}, QJsonValue::Object, "工单操作成功", "工单操作失败").isEmpty();
}


QString ApiClient::diagnose(const AiDiagnosisInput& input) {
    const auto relatedType = input.relatedType.trimmed().isEmpty() ? QStringLiteral("alert") : input.relatedType.trimmed();
    const auto relatedId = input.relatedId.trimmed();
    const auto prompt = input.prompt.trimmed();
    if (token_.isEmpty()) {
        statusMessage_ = "请先登录后端再使用 AI 诊断，已显示离线兜底建议";
        return offlineAiDiagnosis(input);
    }
    if (relatedId.isEmpty() || prompt.isEmpty()) {
        statusMessage_ = "AI 诊断需要关联对象和问题描述";
        return {};
    }

    QStringList contextItems = input.contextItems;
    appendContextItem(contextItems, "设备", input.assetId);
    appendContextItem(contextItems, "告警", input.alertTitle);
    appendContextItem(contextItems, "运行状态", input.runtimeState);
    appendContextItem(contextItems, "严重度", input.severity);
    appendContextItem(contextItems, "指标", input.metricSummary);
    appendContextItem(contextItems, "工单历史", input.workOrderHistory);
    appendContextItem(contextItems, "人工描述", input.operatorDescription);

    QJsonArray contextArray;
    for (const auto& item : contextItems) {
        const auto normalized = item.trimmed();
        if (!normalized.isEmpty()) {
            contextArray.push_back(normalized);
        }
    }

    QJsonObject context;
    context["assetId"] = input.assetId.trimmed();
    context["alertTitle"] = input.alertTitle.trimmed();
    context["runtimeState"] = input.runtimeState.trimmed();
    context["severity"] = input.severity.trimmed();
    context["metricSummary"] = input.metricSummary.trimmed();
    context["workOrderHistory"] = input.workOrderHistory.trimmed();
    context["operatorDescription"] = input.operatorDescription.trimmed();
    context["contextItems"] = contextArray;

    QJsonObject payload;
    payload["relatedType"] = relatedType;
    payload["relatedId"] = relatedId;
    payload["prompt"] = prompt;
    payload["context"] = context;

    const auto envelope = postEnvelope("/api/v1/ai/diagnose", payload, QJsonValue::Object, "AI 诊断已返回", "AI 诊断请求失败");
    if (envelope.isEmpty()) {
        return offlineAiDiagnosis(input);
    }
    return diagnosisReport(envelope.value("data").toObject());
}

QVector<TableRow> ApiClient::aiInteractions(const QString& relatedType, const QString& relatedId) {
    if (token_.isEmpty()) {
        statusMessage_ = "请先登录后端再查看 AI 交互审计，已显示离线演示记录";
        return offlineAiInteractions();
    }

    QString path = "/api/v1/ai/interactions";
    QUrlQuery query;
    const auto normalizedType = relatedType.trimmed();
    const auto normalizedId = relatedId.trimmed();
    if (!normalizedType.isEmpty()) {
        query.addQueryItem("relatedType", normalizedType);
    }
    if (!normalizedId.isEmpty()) {
        query.addQueryItem("relatedId", normalizedId);
    }
    if (!query.isEmpty()) {
        path += "?" + query.toString(QUrl::FullyEncoded);
    }

    const auto envelope = responseEnvelope(path, QJsonValue::Array);
    if (envelope.isEmpty()) {
        statusMessage_ = "AI 交互审计同步失败，已显示离线演示记录";
        return offlineAiInteractions();
    }

    QVector<TableRow> rows;
    const auto interactions = envelope.value("data").toArray();
    for (const auto& value : interactions) {
        const auto item = value.toObject();
        rows.push_back(TableRow{{
            stringValue(item, "id"),
            stringValue(item, "relatedType"),
            stringValue(item, "relatedId"),
            stringValue(item, "input"),
            stringValue(item, "output")}});
    }
    statusMessage_ = rows.isEmpty() ? QStringLiteral("当前关联对象暂无 AI 交互审计") : QStringLiteral("AI 交互审计已同步");
    return rows;
}
QString ApiClient::aiUnavailableMessage() const {
    return "当前客户端已接入登录、资产、运行监控、告警、工单和 AI 诊断入口；AI Provider 不可用时会展示降级建议，核心告警和工单流程不受影响。";
}

QVector<TableRow> ApiClient::offlineAssets() const {
    return {{{"asset-001", "一号产线主电机", "motor", "一号产线", "维护中"}}};
}

QVector<TableRow> ApiClient::offlineMonitoringStates() const {
    return {{{"asset-001", "warning", "温度偏高", "now"}}};
}

QVector<TableRow> ApiClient::offlineAlerts() const {
    return {{{"alert-001", "critical", "asset-001", "温度异常", "已分派", "maintainer"}}};
}

QVector<TableRow> ApiClient::offlineWorkOrders() const {
    return {{{"wo-from-alert-001", "asset-001", "alert-001", "processing", "maintainer", "温度异常现场处理"}}};
}
QVector<TableRow> ApiClient::offlineAiInteractions() const {
    return {{{"ai-interaction-demo", "alert", "alert-001", "温度异常上下文", "离线兜底诊断建议已生成，等待后端审计同步"}}};
}
QString ApiClient::offlineAiDiagnosis(const AiDiagnosisInput& input) const {
    const auto relatedId = input.relatedId.trimmed().isEmpty() ? QStringLiteral("未指定对象") : input.relatedId.trimmed();
    const auto prompt = input.prompt.trimmed().isEmpty() ? QStringLiteral("未填写问题描述") : input.prompt.trimmed();
    QString report;
    report += "AI 诊断（离线兜底）\n\n";
    report += QStringLiteral("关联对象：") + relatedId + QStringLiteral("\n");
    report += QStringLiteral("问题描述：") + prompt + QStringLiteral("\n");
    report += "风险级别：warning\n";
    report += "人审要求：需要人工复核\n\n";
    report += "可能原因\n";
    report += "- AI 后端或 Provider 当前不可用\n";
    report += "- 需要结合告警、运行状态和工单记录继续排查\n\n";
    report += "建议动作\n";
    report += "- 先确认设备实时状态和最近告警\n";
    report += "- 检查维护工单是否已经开始处理或完成\n";
    report += "- 保留人工复核，避免把降级建议作为自动处置依据";
    return report;
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

QJsonObject ApiClient::postEnvelope(const QString& path, const QJsonObject& payload, QJsonValue::Type dataType, const QString& successMessage, const QString& failureMessage) {
    const auto response = requestJson("POST", path, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    if (response.isEmpty()) {
        return {};
    }

    const auto envelope = QJsonDocument::fromJson(response).object();
    const auto data = envelope.value("data");
    if (!envelope.value("success").toBool(false) || data.type() != dataType) {
        if (envelope.value("code").toString().startsWith("AUTH")) {
            token_.clear();
        }
        statusMessage_ = envelope.value("message").toString(failureMessage.isEmpty() ? QStringLiteral("请求失败") : failureMessage);
        return {};
    }
    statusMessage_ = successMessage.isEmpty() ? QStringLiteral("请求成功") : successMessage;
    return envelope;
}

QUrl ApiClient::endpoint(const QString& path) const {
    auto url = apiBaseUrl_;
    auto basePath = url.path();
    if (basePath.endsWith('/')) {
        basePath.chop(1);
    }

    const auto queryStart = path.indexOf('?');
    const auto pathOnly = queryStart >= 0 ? path.left(queryStart) : path;
    url.setPath(basePath + pathOnly);
    if (queryStart >= 0) {
        url.setQuery(path.mid(queryStart + 1));
    }
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
