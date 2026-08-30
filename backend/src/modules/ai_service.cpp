#include "induspilot/modules/ai_service.hpp"

#include "induspilot/data/in_memory_repositories.hpp"

#ifdef INDUSPILOT_WITH_DROGON
#include <drogon/drogon.h>
#endif

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cstddef>
#include <sstream>
#include <string_view>
#include <utility>

namespace induspilot::modules {
namespace {

bool containsText(const std::string& text, const std::string& keyword) {
    return text.find(keyword) != std::string::npos;
}

std::string joinLines(const std::vector<std::string>& items) {
    std::ostringstream out;
    for (std::size_t i = 0; i < items.size(); ++i) {
        if (i > 0) {
            out << "\n";
        }
        out << "- " << items[i];
    }
    return out.str();
}

bool sensitiveKeyAt(const std::string& text, std::size_t position, std::size_t& keyLength) {
    static constexpr std::array<std::string_view, 7> sensitiveKeys{
        "authorization", "api_key", "credential", "password", "secret", "token", "api-key"};
    const auto boundary = [](unsigned char ch) {
        return std::isalnum(ch) || ch == '_' || ch == '-';
    };
    if (position > 0 && boundary(static_cast<unsigned char>(text[position - 1]))) {
        return false;
    }
    for (const auto key : sensitiveKeys) {
        if (position + key.size() > text.size()) {
            continue;
        }
        bool matches = true;
        for (std::size_t offset = 0; offset < key.size(); ++offset) {
            const auto actual = static_cast<unsigned char>(text[position + offset]);
            if (static_cast<char>(std::tolower(actual)) != key[offset]) {
                matches = false;
                break;
            }
        }
        if (matches && (position + key.size() == text.size() || !boundary(static_cast<unsigned char>(text[position + key.size()])))) {
            keyLength = key.size();
            return true;
        }
    }
    return false;
}

std::string redactSensitiveText(const std::string& text) {
    std::string redacted;
    redacted.reserve(text.size());
    for (std::size_t position = 0; position < text.size();) {
        std::size_t keyLength = 0;
        if (!sensitiveKeyAt(text, position, keyLength)) {
            redacted.push_back(text[position++]);
            continue;
        }

        auto valueStart = position + keyLength;
        while (valueStart < text.size() && std::isspace(static_cast<unsigned char>(text[valueStart]))) {
            ++valueStart;
        }
        if (valueStart < text.size() && (text[valueStart] == '"' || text[valueStart] == '\'')) {
            ++valueStart;
            while (valueStart < text.size() && std::isspace(static_cast<unsigned char>(text[valueStart]))) {
                ++valueStart;
            }
        }
        if (valueStart >= text.size() || (text[valueStart] != ':' && text[valueStart] != '=')) {
            redacted.append(text, position, keyLength);
            position += keyLength;
            continue;
        }
        ++valueStart;
        while (valueStart < text.size() && std::isspace(static_cast<unsigned char>(text[valueStart]))) {
            ++valueStart;
        }
        redacted.append(text, position, valueStart - position);
        if (valueStart < text.size() && (text[valueStart] == '"' || text[valueStart] == '\'')) {
            const auto quote = text[valueStart++];
            redacted += "[REDACTED]";
            while (valueStart < text.size()) {
                const auto escaped = text[valueStart] == '\\' && valueStart + 1 < text.size();
                if (text[valueStart] == quote && !escaped) {
                    redacted.push_back(quote);
                    ++valueStart;
                    break;
                }
                ++valueStart;
            }
        } else {
            redacted += "[REDACTED]";
            while (valueStart < text.size() && !std::isspace(static_cast<unsigned char>(text[valueStart])) &&
                   text[valueStart] != ',' && text[valueStart] != ';' && text[valueStart] != ']' && text[valueStart] != '}') {
                ++valueStart;
            }
        }
        position = valueStart;
    }
    return redacted;
}

std::string summarizeContext(const DiagnosisRequest& request) {
    std::ostringstream out;
    out << "relatedType=" << request.relatedType << "\n";
    out << "relatedId=" << request.relatedId << "\n";
    out << "prompt=" << redactSensitiveText(request.prompt) << "\n";
    out << "assetId=" << request.context.assetId << "\n";
    out << "alertTitle=" << request.context.alertTitle << "\n";
    out << "runtimeState=" << request.context.runtimeState << "\n";
    out << "severity=" << request.context.severity << "\n";
    out << "metricSummary=" << redactSensitiveText(request.context.metricSummary) << "\n";
    out << "workOrderHistory=" << redactSensitiveText(request.context.workOrderHistory) << "\n";
    out << "operatorDescription=" << redactSensitiveText(request.context.operatorDescription) << "\n";
    std::vector<std::string> redactedItems;
    redactedItems.reserve(request.context.contextItems.size());
    for (const auto& item : request.context.contextItems) {
        redactedItems.push_back(redactSensitiveText(item));
    }
    out << "contextItems=" << joinLines(redactedItems);
    return out.str();
}

std::string serializeDiagnosis(const DiagnosisResult& result) {
    std::ostringstream out;
    out << "provider=" << result.provider << "\n";
    out << "available=" << (result.available ? "true" : "false") << "\n";
    out << "riskLevel=" << result.riskLevel << "\n";
    out << "requiresHumanReview=" << (result.requiresHumanReview ? "true" : "false") << "\n";
    out << "summary=" << result.summary << "\n";
    out << "possibleCauses:\n" << joinLines(result.possibleCauses) << "\n";
    out << "recommendedActions:\n" << joinLines(result.recommendedActions) << "\n";
    out << "rawProviderOutput=" << result.rawProviderOutput;
    return out.str();
}

std::string mergedDiagnosisText(const DiagnosisRequest& request) {
    std::ostringstream out;
    out << request.prompt << ' ' << request.context.alertTitle << ' ' << request.context.metricSummary << ' '
        << request.context.operatorDescription << ' ' << request.context.runtimeState << ' ' << request.context.severity;
    for (const auto& item : request.context.contextItems) {
        out << ' ' << item;
    }
    return out.str();
}

std::string chooseRiskLevel(const DiagnosisRequest& request) {
    const auto text = mergedDiagnosisText(request);
    if (request.context.severity == "critical" || request.context.runtimeState == "critical" || containsText(text, "停机") || containsText(text, "高温") || containsText(text, "critical")) {
        return "critical";
    }
    if (request.context.severity == "warning" || request.context.runtimeState == "warning" || containsText(text, "异常") || containsText(text, "warning")) {
        return "warning";
    }
    return "info";
}

std::vector<std::string> inferCauses(const DiagnosisRequest& request) {
    const auto text = mergedDiagnosisText(request);
    std::vector<std::string> causes;
    if (containsText(text, "温度") || containsText(text, "高温") || containsText(text, "过热")) {
        causes.push_back("冷却、润滑或负载异常导致设备温度升高");
        causes.push_back("温度传感器漂移或采样点位异常");
    }
    if (containsText(text, "振动") || containsText(text, "震动")) {
        causes.push_back("轴承磨损、联轴器偏心或地脚松动引发振动异常");
    }
    if (containsText(text, "离线") || containsText(text, "offline")) {
        causes.push_back("设备供电、网络链路或采集网关连接异常");
    }
    if (containsText(text, "告警") || containsText(text, "异常")) {
        causes.push_back("工况波动触发阈值规则，需要结合运行趋势确认是否为持续异常");
    }
    if (causes.empty()) {
        causes.push_back("现场工况、传感器数据或维护状态存在变化，需要补充上下文确认根因");
    }
    return causes;
}

std::vector<std::string> recommendActions(const DiagnosisRequest& request, const std::string& riskLevel) {
    std::vector<std::string> actions;
    if (riskLevel == "critical") {
        actions.push_back("立即通知现场负责人评估是否降载或停机，优先保障人员与设备安全");
    }
    actions.push_back("核对最近一次运行状态、告警时间和传感器原始值，确认异常是否持续");
    actions.push_back("检查资产 " + (request.context.assetId.empty() ? request.relatedId : request.context.assetId) + " 的冷却、供电、网络和机械连接状态");
    actions.push_back("关联近期维护工单和更换记录，判断是否存在重复故障或维修后复发");
    actions.push_back("将排查结论记录到工单，并在处理后复核告警是否恢复正常");
    return actions;
}

struct ParsedHttpEndpoint {
    std::string baseUrl;
    std::string path{"/"};
    bool valid{false};
};

ParsedHttpEndpoint parseHttpEndpoint(const std::string& endpoint) {
    const auto schemePos = endpoint.find("://");
    if (schemePos == std::string::npos) {
        return {};
    }
    const auto pathPos = endpoint.find('/', schemePos + 3);
    ParsedHttpEndpoint parsed;
    parsed.valid = true;
    if (pathPos == std::string::npos) {
        parsed.baseUrl = endpoint;
        return parsed;
    }
    parsed.baseUrl = endpoint.substr(0, pathPos);
    parsed.path = endpoint.substr(pathPos);
    if (parsed.path.empty()) {
        parsed.path = "/";
    }
    return parsed;
}

std::vector<std::string> limitedContextItems(const AiProviderRequest& request, int maxContextItems) {
    if (maxContextItems <= 0) {
        return {};
    }
    auto items = request.contextItems;
    if (items.size() > static_cast<std::size_t>(maxContextItems)) {
        items.resize(static_cast<std::size_t>(maxContextItems));
    }
    return items;
}

#ifdef INDUSPILOT_WITH_DROGON
Json::Value providerRequestToJson(const AiProviderRequest& request, int maxContextItems) {
    Json::Value value;
    value["operation"] = request.operation;
    value["prompt"] = redactSensitiveText(request.prompt);
    Json::Value context(Json::arrayValue);
    for (const auto& item : limitedContextItems(request, maxContextItems)) {
        context.append(redactSensitiveText(item));
    }
    value["contextItems"] = context;
    return value;
}

std::string textField(const Json::Value& value, const char* field) {
    if (value.isMember(field) && value[field].isString()) {
        return value[field].asString();
    }
    return {};
}

std::string extractProviderContent(const Json::Value& value) {
    for (const auto* field : {"content", "summary", "text", "output_text"}) {
        const auto text = textField(value, field);
        if (!text.empty()) {
            return text;
        }
    }

    if (value.isMember("choices") && value["choices"].isArray() && !value["choices"].empty()) {
        const auto& choice = value["choices"][0];
        if (choice.isMember("message")) {
            const auto text = textField(choice["message"], "content");
            if (!text.empty()) {
                return text;
            }
        }
        const auto text = textField(choice, "text");
        if (!text.empty()) {
            return text;
        }
    }

    if (value.isMember("output") && value["output"].isArray()) {
        for (const auto& output : value["output"]) {
            if (!output.isMember("content") || !output["content"].isArray()) {
                continue;
            }
            for (const auto& content : output["content"]) {
                const auto text = textField(content, "text");
                if (!text.empty()) {
                    return text;
                }
            }
        }
    }

    return {};
}

bool isRetryableStatus(int statusCode) {
    return statusCode == 408 || statusCode == 429 || statusCode >= 500;
}
#endif
class DisabledAiProvider final : public AiProvider {
public:
    ServiceStatus status() const override {
        return ServiceStatus{"ai-provider", true, "disabled provider：使用本地规则编排生成可审计诊断"};
    }

    AiProviderResult complete(const AiProviderRequest& request) const override {
        return AiProviderResult{false, "disabled", "未调用外部模型；已按 " + request.operation + " 使用本地规则编排生成诊断"};
    }
};

class HttpAiProvider final : public AiProvider {
public:
    explicit HttpAiProvider(app::AiConfig config)
        : config_(std::move(config)) {}

    ServiceStatus status() const override {
        const auto parsed = parseHttpEndpoint(config_.endpoint);
        if (!parsed.valid) {
            return ServiceStatus{"ai-provider", false, "http provider endpoint 格式无效：" + config_.endpoint};
        }
#ifdef INDUSPILOT_WITH_DROGON
        return ServiceStatus{"ai-provider", true, "http provider 已启用真实推理传输 endpoint=" + config_.endpoint + (config_.apiKey.empty() ? "；未配置鉴权" : "；已配置鉴权头 " + config_.authHeader)};
#else
        return ServiceStatus{"ai-provider", false, "http provider 已配置 endpoint=" + config_.endpoint + "，但当前构建未启用 Drogon HTTP 传输"};
#endif
    }

    AiProviderResult complete(const AiProviderRequest& request) const override {
        const auto parsed = parseHttpEndpoint(config_.endpoint);
        if (!parsed.valid) {
            return AiProviderResult{false, "http", "HTTP provider endpoint 格式无效，已按 " + request.operation + " 使用本地规则降级"};
        }
#ifndef INDUSPILOT_WITH_DROGON
        return AiProviderResult{false, "http", "当前构建未启用 Drogon HTTP 传输，已按 " + request.operation + " 使用本地规则降级"};
#else
        try {
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds((std::max)(config_.timeoutMs, 1));
            std::string lastFailure = "HTTP provider 调用失败";
            for (int attempt = 0; attempt <= config_.maxRetries; ++attempt) {
                const auto now = std::chrono::steady_clock::now();
                if (now >= deadline) {
                    break;
                }
                const auto remainingMs = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count();
                auto client = drogon::HttpClient::newHttpClient(parsed.baseUrl);
                auto httpRequest = drogon::HttpRequest::newHttpJsonRequest(providerRequestToJson(request, config_.maxContextItems));
                httpRequest->setMethod(drogon::Post);
                httpRequest->setPath(parsed.path);
                httpRequest->addHeader("X-IndusPilot-Ai-Operation", request.operation);
                if (!config_.apiKey.empty() && !config_.authHeader.empty()) {
                    const auto authValue = config_.authScheme.empty() ? config_.apiKey : config_.authScheme + " " + config_.apiKey;
                    httpRequest->addHeader(config_.authHeader, authValue);
                }

                const auto timeoutSeconds = static_cast<double>((std::max)(remainingMs, 1LL)) / 1000.0;
                const auto responsePair = client->sendRequest(httpRequest, timeoutSeconds);
                if (responsePair.first != drogon::ReqResult::Ok || !responsePair.second) {
                    lastFailure = "HTTP provider 调用失败";
                } else {
                    const auto statusCode = static_cast<int>(responsePair.second->statusCode());
                    const auto body = std::string(responsePair.second->getBody());
                    if (statusCode >= 200 && statusCode < 300) {
                        if (body.size() > static_cast<std::size_t>(config_.maxResponseBytes)) {
                            return AiProviderResult{false, "http", "HTTP provider 响应超过大小限制，已按 " + request.operation + " 使用本地规则降级"};
                        }
                        const auto json = responsePair.second->getJsonObject();
                        if (json) {
                            const auto content = extractProviderContent(*json);
                            if (content.empty()) {
                                return AiProviderResult{false, "http", "HTTP provider 响应缺少可用文本字段，已按 " + request.operation + " 使用本地规则降级"};
                            }
                            return AiProviderResult{true, "http", content};
                        }
                        if (config_.requireStructuredResponse) {
                            return AiProviderResult{false, "http", "HTTP provider 响应不是 JSON，已按 " + request.operation + " 使用本地规则降级"};
                        }
                        if (body.empty()) {
                            return AiProviderResult{false, "http", "HTTP provider 响应为空，已按 " + request.operation + " 使用本地规则降级"};
                        }
                        return AiProviderResult{true, "http", body};
                    }
                    lastFailure = "HTTP provider 返回状态码 " + std::to_string(statusCode);
                    if (!isRetryableStatus(statusCode)) {
                        break;
                    }
                }
            }
            return AiProviderResult{false, "http", lastFailure + "，已按 " + request.operation + " 使用本地规则降级"};
        } catch (const std::exception& ex) {
            return AiProviderResult{false, "http", std::string("HTTP provider 异常：") + ex.what() + "，已按 " + request.operation + " 使用本地规则降级"};
        }
#endif
    }

private:
    app::AiConfig config_;
};

}  // namespace

std::shared_ptr<AiProvider> makeAiProvider(const app::AiConfig& config) {
    if (config.enabled && config.provider == "http") {
        return std::make_shared<HttpAiProvider>(config);
    }
    return std::make_shared<DisabledAiProvider>();
}

AiService::AiService(app::AiConfig config, std::shared_ptr<data::AiInteractionRepository> repository, std::shared_ptr<AiProvider> provider, std::shared_ptr<MetricsRegistry> metrics)
    : config_(std::move(config)), repository_(std::move(repository)), provider_(std::move(provider)), metrics_(std::move(metrics)) {
    if (!repository_) {
        repository_ = std::make_shared<data::InMemoryAiInteractionRepository>();
    }
    if (!provider_) {
        provider_ = makeAiProvider(config_);
    }
}

ServiceStatus AiService::status() const {
    const auto providerStatus = provider_->status();
    if (!config_.enabled) {
        return ServiceStatus{"ai-diagnosis-assistance", true, "AI 未启用；" + providerStatus.message};
    }
    return ServiceStatus{"ai-diagnosis-assistance", providerStatus.ready, "AI provider=" + config_.provider + "；" + providerStatus.message};
}

std::string AiService::providerName() const {
    return config_.enabled ? config_.provider : "disabled";
}

std::string AiService::providerEndpoint() const {
    return config_.endpoint;
}

AiProviderResult AiService::completeProvider(const AiProviderRequest& request) {
    const auto startedAt = std::chrono::steady_clock::now();
    const auto result = provider_->complete(request);
    if (metrics_) {
        const auto elapsed = std::chrono::steady_clock::now() - startedAt;
        const auto durationMs = static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count()) / 1000.0;
        metrics_->recordAiProviderCall(result.provider, request.operation, result.available, durationMs);
    }
    return result;
}

AiSuggestion AiService::explainAlert(const std::string& alertSummary) {
    return troubleshoot(AiRequest{"alert", "unknown", alertSummary, {alertSummary}});
}

AiSuggestion AiService::troubleshoot(const AiRequest& request) {
    return unavailableSuggestion(request, "故障排查");
}

AiSuggestion AiService::summarizeLogs(const AiRequest& request) {
    return unavailableSuggestion(request, "日志摘要");
}

DiagnosisResult AiService::diagnose(const DiagnosisRequest& request) {
    auto providerResult = completeProvider(AiProviderRequest{"diagnose", request.prompt, request.context.contextItems});
    DiagnosisResult result;
    result.available = providerResult.available;
    result.provider = providerResult.provider;
    result.riskLevel = chooseRiskLevel(request);
    result.requiresHumanReview = result.riskLevel != "info";
    result.summary = "Agent 已整合告警、运行状态、工单历史和人工描述，生成 " + result.riskLevel + " 风险诊断建议";
    result.possibleCauses = inferCauses(request);
    result.recommendedActions = recommendActions(request, result.riskLevel);
    result.rawProviderOutput = providerResult.content;
    recordDiagnosis(request, result);
    return result;
}

std::vector<domain::AiInteraction> AiService::interactions(const AiInteractionQuery& query) const {
    return interactionsPage(query).interactions;
}

AiInteractionPage AiService::interactionsPage(const AiInteractionQuery& query) const {
    return repository_->list(data::AiInteractionRepository::Query{
        query.relatedType,
        query.relatedId,
        query.limit,
        query.offset});
}

AiSuggestion AiService::unavailableSuggestion(const AiRequest& request, const std::string& operation) {
    const auto providerResult = completeProvider(AiProviderRequest{operation, request.prompt, request.contextItems});
    const auto content = config_.enabled
        ? "已记录" + operation + "请求；" + providerResult.content + "，核心流程可继续执行"
        : "AI 未启用，已记录" + operation + "请求，核心流程可继续执行";
    AiSuggestion suggestion{providerResult.available, "辅助建议", content};
    recordInteraction(request, suggestion);
    return suggestion;
}

void AiService::recordInteraction(const AiRequest& request, const AiSuggestion& suggestion) {
    if (!config_.storeInteractionRecords) {
        return;
    }
    std::ostringstream id;
    id << "ai-interaction-" << repository_->list().size() + 1;
    repository_->save(domain::AiInteraction{id.str(), request.relatedType, request.relatedId, redactSensitiveText(request.prompt), suggestion.content});
}

void AiService::recordDiagnosis(const DiagnosisRequest& request, const DiagnosisResult& result) {
    if (!config_.storeInteractionRecords) {
        return;
    }
    std::ostringstream id;
    id << "ai-interaction-" << repository_->list().size() + 1;
    repository_->save(domain::AiInteraction{id.str(), request.relatedType, request.relatedId, summarizeContext(request), serializeDiagnosis(result)});
}

}  // namespace induspilot::modules
