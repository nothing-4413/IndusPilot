#pragma once

#include "induspilot/api/api_types.hpp"
#include "induspilot/api/router.hpp"
#include "induspilot/app/config.hpp"
#include "induspilot/data/data_connectors.hpp"

#include <map>
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

    AppConfig config_;
    data::DependencyStatus dependencies_;
    ConfigValidation configValidation_;
    api::Router router_;
    bool initialized_{false};
    bool running_{false};
};

}  // namespace induspilot::app
