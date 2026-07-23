#include "induspilot/modules/ai_service.hpp"

#include <sstream>

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

ServiceStatus AiService::status() const {
    return ServiceStatus{"ai-diagnosis-assistance", true, "AI assistance audit service is ready"};
}

AiSuggestion AiService::explainAlert(const std::string& alertSummary) {
    return troubleshoot(AiRequest{"alert", "unknown", alertSummary, {alertSummary}});
}

AiSuggestion AiService::troubleshoot(const AiRequest& request) {
    return unavailableSuggestion(request, "troubleshooting");
}

AiSuggestion AiService::summarizeLogs(const AiRequest& request) {
    return unavailableSuggestion(request, "log summary");
}

std::vector<domain::AiInteraction> AiService::interactions(const AiInteractionQuery& query) const {
    std::vector<domain::AiInteraction> result;
    for (const auto& interaction : interactions_) {
        if (matches(interaction, query)) {
            result.push_back(interaction);
        }
    }
    return result;
}

AiSuggestion AiService::unavailableSuggestion(const AiRequest& request, const std::string& operation) {
    AiSuggestion suggestion{false, "assistance", "AI service is unavailable; recorded " + operation + " request and core workflow can continue"};
    recordInteraction(request, suggestion);
    return suggestion;
}

void AiService::recordInteraction(const AiRequest& request, const AiSuggestion& suggestion) {
    std::ostringstream id;
    id << "ai-interaction-" << interactions_.size() + 1;
    interactions_.push_back(domain::AiInteraction{id.str(), request.relatedType, request.relatedId, request.prompt, suggestion.content});
}

}  // namespace induspilot::modules