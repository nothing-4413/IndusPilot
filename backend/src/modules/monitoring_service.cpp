#include "induspilot/modules/monitoring_service.hpp"

#include "induspilot/data/in_memory_repositories.hpp"

#include <array>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <utility>

namespace induspilot::modules {
namespace {

std::string currentTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &time);
#else
    localtime_r(&time, &localTime);
#endif
    std::ostringstream out;
    out << std::put_time(&localTime, "%Y-%m-%dT%H:%M:%S");
    return out.str();
}

template <std::size_t Size>
bool contains(const std::array<const char*, Size>& values, const std::string& value) {
    for (const auto* item : values) {
        if (value == item) {
            return true;
        }
    }
    return false;
}

}  // namespace

bool isSupportedRuntimeState(const std::string& state) {
    static constexpr std::array<const char*, 4> states{"online", "warning", "critical", "offline"};
    return contains(states, state);
}

bool isSupportedRuntimeSeverity(const std::string& severity) {
    static constexpr std::array<const char*, 3> severities{"info", "warning", "critical"};
    return contains(severities, severity);
}

MonitoringService::MonitoringService() : MonitoringService(std::make_shared<data::InMemoryRuntimeStateRepository>()) {}

MonitoringService::MonitoringService(std::shared_ptr<data::RuntimeStateRepository> repository) : repository_(std::move(repository)) {
    if (!repository_) {
        repository_ = std::make_shared<data::InMemoryRuntimeStateRepository>();
    }
}

ServiceStatus MonitoringService::status() const {
    return ServiceStatus{"operational-monitoring", true, "runtime state repository is ready"};
}

RuntimeState MonitoringService::updateState(RuntimeState state) {
    if (state.updatedAt.empty()) {
        state.updatedAt = currentTimestamp();
    }
    if (state.severity.empty()) {
        state.severity = "info";
    }
    return repository_->save(std::move(state));
}

std::optional<RuntimeState> MonitoringService::findState(const std::string& assetId) const {
    return repository_->findByAssetId(assetId);
}

std::vector<RuntimeState> MonitoringService::listStates() const {
    return repository_->list();
}

std::map<std::string, int> MonitoringService::summarizeStates() const {
    std::map<std::string, int> summary{{"online", 0}, {"warning", 0}, {"critical", 0}, {"offline", 0}};
    for (const auto& state : repository_->list()) {
        summary[state.state]++;
    }
    return summary;
}

std::map<std::string, int> MonitoringService::summarizeSeverity() const {
    std::map<std::string, int> summary{{"info", 0}, {"warning", 0}, {"critical", 0}};
    for (const auto& state : repository_->list()) {
        summary[state.severity]++;
    }
    return summary;
}

}  // namespace induspilot::modules