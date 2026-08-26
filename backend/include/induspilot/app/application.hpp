#pragma once

#include "induspilot/api/api_types.hpp"
#include "induspilot/api/router.hpp"
#include "induspilot/app/config.hpp"
#include "induspilot/data/data_connectors.hpp"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace induspilot::app {

class Application {
public:
    explicit Application(AppConfig config);

    bool start();
    void stop();
    bool isRunning() const;
    bool isInitialized() const;
    api::HealthCheck health() const;
    struct StartupStatus {
        bool configurationValid{true};
        bool initialized{false};
        std::vector<std::string> errors;
    };
    struct ReadinessStatus {
        bool ready{false};
        std::map<std::string, data::DependencyCheck> dependencies;
        bool probeInProgress{false};
        std::uint64_t probeCount{0};
        std::uint64_t failureCount{0};
        std::uint64_t recoveryCount{0};
        std::int64_t lastProbeDurationMs{0};
        std::int64_t lastProbeAtUnixMs{0};
    };
    StartupStatus startup() const;
    ReadinessStatus readiness() const;
    api::Router& router();

private:
    void registerRoutes();
    void refreshDependencies() const;
    ReadinessStatus readinessLocked() const;

    AppConfig config_;
    mutable data::DependencyStatus dependencies_;
    mutable data::DependencyRequirements requirements_;
    mutable ConfigValidation configValidation_;
    api::Router router_;
    mutable bool initialized_{false};
    mutable bool running_{false};
    mutable bool hasProbe_{false};
    mutable bool probeInFlight_{false};
    mutable std::uint64_t probeCount_{0};
    mutable std::uint64_t failureCount_{0};
    mutable std::uint64_t recoveryCount_{0};
    mutable std::int64_t lastProbeDurationMs_{0};
    mutable std::int64_t lastProbeAtUnixMs_{0};
    mutable std::chrono::steady_clock::time_point lastProbeAt_{};
    mutable std::mutex stateMutex_;
    mutable std::condition_variable probeCondition_;
};

}  // namespace induspilot::app
