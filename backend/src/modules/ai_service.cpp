#include "induspilot/modules/ai_service.hpp"

#include <sstream>

namespace induspilot::modules {

ServiceStatus AiService::status() const {
    return ServiceStatus{"ai-diagnosis-assistance", true, "AI 辅助诊断模块占位就绪"};
}

AiSuggestion AiService::explainAlert(const std::string& alertSummary) {
    return troubleshoot(AiRequest{"alert", "unknown", alertSummary, {alertSummary}});
}

AiSuggestion AiService::troubleshoot(const AiRequest& request) {
    return unavailableSuggestion(request, "故障排查建议");
}

AiSuggestion AiService::summarizeLogs(const AiRequest& request) {
    return unavailableSuggestion(request, "日志摘要");
}

std::vector<domain::AiInteraction> AiService::interactions() const {
    return interactions_;
}

AiSuggestion AiService::unavailableSuggestion(const AiRequest& request, const std::string& operation) {
    AiSuggestion suggestion{false, "辅助建议", "AI 服务未启用，已记录 " + operation + " 请求，核心流程可继续执行"};
    recordInteraction(request, suggestion);
    return suggestion;
}

void AiService::recordInteraction(const AiRequest& request, const AiSuggestion& suggestion) {
    std::ostringstream id;
    id << "ai-interaction-" << interactions_.size() + 1;
    interactions_.push_back(domain::AiInteraction{id.str(), request.relatedType, request.relatedId, request.prompt, suggestion.content});
}

}  // namespace induspilot::modules