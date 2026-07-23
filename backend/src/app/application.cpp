#include "induspilot/app/application.hpp"

#include "induspilot/api/api_types.hpp"

#include <utility>

namespace induspilot::app {

Application::Application(AppConfig config) : config_(std::move(config)) {
    registerRoutes();
}

void Application::start() {
    dependencies_ = data::DataConnectors{config_}.probe();
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
        health.warnings.push_back("依赖健康检查当前仅验证 TCP 连通性，尚未校验认证、库表结构或集合状态");
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