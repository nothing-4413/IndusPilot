#include "induspilot/modules/ai_service.hpp"

namespace induspilot::modules {

ServiceStatus AiService::status() const {
    return ServiceStatus{"ai-diagnosis-assistance", true, "AI 辅助诊断模块占位就绪"};
}

AiSuggestion AiService::explainAlert(const std::string& alertSummary) const {
    if (alertSummary.empty()) {
        return AiSuggestion{false, "辅助建议", "缺少告警上下文，暂不能生成建议"};
    }
    return AiSuggestion{false, "辅助建议", "AI 服务未启用，已保留接口边界"};
}

}  // namespace induspilot::modules