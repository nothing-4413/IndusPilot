#pragma once

#include "induspilot/api/api_types.hpp"
#include "induspilot/api/router.hpp"
#include "induspilot/app/config.hpp"
#include "induspilot/data/data_connectors.hpp"

#include <chrono>
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
    };
    StartupStatus startup() const;
    ReadinessStatus readiness() const;
    api::Router& router();

private:
    void registerRoutes();
    void refreshDependenciesLocked() const;

    AppConfig config_;
    mutable data::DependencyStatus dependencies_;
    mutable data::DependencyRequirements requirements_;
    mutable ConfigValidation configValidation_;
    api::Router router_;
    mutable bool initialized_{false};
    mutable bool running_{false};
    mutable bool hasProbe_{false};
    mutable std::chrono::steady_clock::time_point lastProbeAt_{};
    mutable std::mutex stateMutex_;
};

}  // namespace induspilot::app
