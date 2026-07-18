#pragma once

#include "induspilot/api/api_types.hpp"
#include "induspilot/api/router.hpp"
#include "induspilot/app/config.hpp"
#include "induspilot/data/data_connectors.hpp"

namespace induspilot::app {

class Application {
public:
    explicit Application(AppConfig config);

    void start();
    void stop();
    bool isRunning() const;
    api::HealthCheck health() const;
    api::Router& router();

private:
    void registerRoutes();

    AppConfig config_;
    data::DependencyStatus dependencies_;
    api::Router router_;
    bool running_{false};
};

}  // namespace induspilot::app