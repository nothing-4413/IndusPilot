#pragma once

#include "induspilot/data/repositories.hpp"
#include "induspilot/domain/domain_types.hpp"
#include "induspilot/modules/service_status.hpp"

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace induspilot::modules {

using RuntimeState = domain::RuntimeState;

class MonitoringService {
public:
    MonitoringService();
    explicit MonitoringService(std::shared_ptr<data::RuntimeStateRepository> repository);

    ServiceStatus status() const;
    RuntimeState updateState(RuntimeState state);
    std::optional<RuntimeState> findState(const std::string& assetId) const;
    std::vector<RuntimeState> listStates() const;
    std::map<std::string, int> summarizeStates() const;
    std::map<std::string, int> summarizeSeverity() const;

private:
    std::shared_ptr<data::RuntimeStateRepository> repository_;
};

bool isSupportedRuntimeState(const std::string& state);
bool isSupportedRuntimeSeverity(const std::string& severity);

}  // namespace induspilot::modules