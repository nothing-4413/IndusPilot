#pragma once

#include "induspilot/app/config.hpp"
#include "induspilot/data/repositories.hpp"
#include "induspilot/domain/domain_types.hpp"
#include "induspilot/modules/service_status.hpp"

#include <memory>
#include <optional>
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

struct AiInteractionQuery {
    std::optional<std::string> relatedType;
    std::optional<std::string> relatedId;
};

struct DiagnosisContext {
    std::string assetId;
    std::string alertTitle;
    std::string runtimeState;
    std::string severity;
    std::string metricSummary;
    std::string workOrderHistory;
    std::string operatorDescription;
    std::vector<std::string> contextItems;
};

struct DiagnosisRequest {
    std::string relatedType{"asset"};
    std::string relatedId;
    std::string prompt;
    DiagnosisContext context;
};

struct DiagnosisResult {
    bool available{false};
    std::string provider{"disabled"};
    std::string summary;
    std::vector<std::string> possibleCauses;
    std::vector<std::string> recommendedActions;
    std::string riskLevel{"info"};
    bool requiresHumanReview{true};
    std::string rawProviderOutput;
};

struct AiProviderRequest {
    std::string operation;
    std::string prompt;
    std::vector<std::string> contextItems;
};

struct AiProviderResult {
    bool available{false};
    std::string provider{"disabled"};
    std::string content;
};

class AiProvider {
public:
    virtual ~AiProvider() = default;

    virtual ServiceStatus status() const = 0;
    virtual AiProviderResult complete(const AiProviderRequest& request) const = 0;
};

std::shared_ptr<AiProvider> makeAiProvider(const app::AiConfig& config);

class AiService {
public:
    explicit AiService(
        app::AiConfig config = app::AiConfig{},
        std::shared_ptr<data::AiInteractionRepository> repository = nullptr,
        std::shared_ptr<AiProvider> provider = nullptr);

    ServiceStatus status() const;
    std::string providerName() const;
    std::string providerEndpoint() const;
    AiSuggestion explainAlert(const std::string& alertSummary);
    AiSuggestion troubleshoot(const AiRequest& request);
    AiSuggestion summarizeLogs(const AiRequest& request);
    DiagnosisResult diagnose(const DiagnosisRequest& request);
    std::vector<domain::AiInteraction> interactions(const AiInteractionQuery& query = {}) const;

private:
    AiSuggestion unavailableSuggestion(const AiRequest& request, const std::string& operation);
    void recordInteraction(const AiRequest& request, const AiSuggestion& suggestion);
    void recordDiagnosis(const DiagnosisRequest& request, const DiagnosisResult& result);

    app::AiConfig config_;
    std::shared_ptr<data::AiInteractionRepository> repository_;
    std::shared_ptr<AiProvider> provider_;
};

}  // namespace induspilot::modules
