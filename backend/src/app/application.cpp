#include "induspilot/app/application.hpp"

#include "induspilot/api/api_types.hpp"

#include <utility>

namespace induspilot::app {

Application::Application(AppConfig config) : config_(std::move(config)) {
    registerRoutes();
}

void Application::start() {
    dependencies_ = data::DataConnectors{}.probe();
    running_ = true;
}

void Application::stop() {
    running_ = false;
}

bool Application::isRunning() const {
    return running_;
}

api::HealthCheck Application::health() const {
    api::HealthCheck health;
    health.dependencies = {
        {"mysql", dependencies_.mysql},
        {"redis", dependencies_.redis},
        {"mongodb", dependencies_.mongodb},
        {"ai", config_.ai.enabled ? dependencies_.ai : true},
    };
    if (!dependencies_.mysql || !dependencies_.redis || !dependencies_.mongodb) {
        health.warnings.push_back("依赖探测当前为占位实现，请在接入真实连接后启用严格检查");
    }
    return health;
}

api::Router& Application::router() {
    return router_;
}

void Application::registerRoutes() {
    router_.addRoute("GET", "/health", [this] {
        return api::ApiResponse{true, "OK", "服务健康状态已生成", api::toJson(health())};
    });
}

}  // namespace induspilot::app