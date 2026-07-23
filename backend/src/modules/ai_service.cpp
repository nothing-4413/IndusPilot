#include "induspilot/modules/ai_service.hpp"

#include "induspilot/data/in_memory_repositories.hpp"

#include <sstream>
#include <utility>

namespace induspilot::modules {
namespace {

bool matches(const domain::AiInteraction& interaction, const AiInteractionQuery& query) {
    if (query.relatedType && interaction.relatedType != *query.relatedType) {
        return false;
    }
    if (query.relatedId && interaction.relatedId != *query.relatedId) {
        return false;
    }
    return true;
}

}  // namespace

AiService::AiService(app::AiConfig config, std::shared_ptr<data::AiInteractionRepository> repository)
    : config_(std::move(config)), repository_(std::move(repository)) {
    if (!repository_) {
        repository_ = std::make_shared<data::InMemoryAiInteractionRepository>();
    }
}

ServiceStatus AiService::status() const {
    if (!config_.enabled) {
        return ServiceStatus{"ai-diagnosis-assistance", true, "AI 未启用，当前仅记录辅助请求审计"};
    }
    return ServiceStatus{"ai-diagnosis-assistance", true, "AI endpoint 已配置为 " + config_.endpoint + "，外部推理调用尚未接入，当前使用审计降级模式"};
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

std::vector<domain::AiInteraction> AiService::interactions(const AiInteractionQuery& query) const {
    std::vector<domain::AiInteraction> result;
    for (const auto& interaction : repository_->list()) {
        if (matches(interaction, query)) {
            result.push_back(interaction);
        }
    }
    return result;
}

AiSuggestion AiService::unavailableSuggestion(const AiRequest& request, const std::string& operation) {
    const auto content = config_.enabled
        ? "已记录" + operation + "请求；外部 AI endpoint 已配置但推理调用尚未接入，核心流程可继续执行"
        : "AI 未启用，已记录" + operation + "请求，核心流程可继续执行";
    AiSuggestion suggestion{false, "辅助建议", content};
    recordInteraction(request, suggestion);
    return suggestion;
}

void AiService::recordInteraction(const AiRequest& request, const AiSuggestion& suggestion) {
    std::ostringstream id;
    id << "ai-interaction-" << repository_->list().size() + 1;
    repository_->save(domain::AiInteraction{id.str(), request.relatedType, request.relatedId, request.prompt, suggestion.content});
}

}  // namespace induspilot::modules