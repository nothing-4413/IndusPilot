#pragma once

#include "induspilot/domain/domain_types.hpp"
#include "induspilot/modules/service_status.hpp"

#include <string>
#include <vector>

namespace induspilot::modules {

struct AiRequest {
    std::string relatedType;
    std::string relatedId;
    std::string prompt;
    std::vector<std::string> contextItems;
};

struct AiSuggestion {
    bool available{false};
    std::string label{"辅助建议"};
    std::string content{"AI 服务未启用，核心流程可继续执行"};
};

class AiService {
public:
    ServiceStatus status() const;
    AiSuggestion explainAlert(const std::string& alertSummary);
    AiSuggestion troubleshoot(const AiRequest& request);
    AiSuggestion summarizeLogs(const AiRequest& request);
    std::vector<domain::AiInteraction> interactions() const;

private:
    AiSuggestion unavailableSuggestion(const AiRequest& request, const std::string& operation);
    void recordInteraction(const AiRequest& request, const AiSuggestion& suggestion);

    std::vector<domain::AiInteraction> interactions_;
};

}  // namespace induspilot::modules