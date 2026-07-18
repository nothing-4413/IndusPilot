#pragma once

#include "induspilot/modules/service_status.hpp"

#include <string>

namespace induspilot::modules {

struct AiSuggestion {
    bool available{false};
    std::string label{"辅助建议"};
    std::string content{"AI 服务未启用，核心流程可继续执行"};
};

class AiService {
public:
    ServiceStatus status() const;
    AiSuggestion explainAlert(const std::string& alertSummary) const;
};

}  // namespace induspilot::modules