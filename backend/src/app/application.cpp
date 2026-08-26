#include "induspilot/app/application.hpp"

#include "induspilot/api/api_types.hpp"

#include <chrono>
#include <mutex>
#include <utility>

namespace induspilot::app {

Application::Application(AppConfig config) : config_(std::move(config)) {
    registerRoutes();
}

bool Application::start() {
    std::lock_guard lock(stateMutex_);
    configValidation_ = validateConfig(config_);
    if (!configValidation_.valid) {
        initialized_ = false;
        running_ = false;
        return false;
    }

    requirements_ = data::DataConnectors{config_}.requirements();
    refreshDependenciesLocked();
    initialized_ = true;
    running_ = true;
    return true;
}

void Application::stop() {
    std::lock_guard lock(stateMutex_);
    running_ = false;
}

bool Application::isRunning() const {
    std::lock_guard lock(stateMutex_);
    return running_;
}

bool Application::isInitialized() const {
    std::lock_guard lock(stateMutex_);
    return initialized_;
}

api::HealthCheck Application::health() const {
    std::lock_guard lock(stateMutex_);
    api::HealthCheck health;
    health.dependencies = {
        {"mysql", dependencies_.mysql.available},
        {"redis", dependencies_.redis.available},
        {"mongodb", dependencies_.mongodb.available},
        {"ai", dependencies_.ai.available},
    };
    const std::map<std::string, data::DependencyCheck> dependencyChecks = {
        {"mysql", dependencies_.mysql},
        {"redis", dependencies_.redis},
        {"mongodb", dependencies_.mongodb},
        {"ai", dependencies_.ai},
    };
    for (const auto& dependency : dependencyChecks) {
        if (dependency.second.required && !dependency.second.available) {
            health.warnings.push_back(dependency.first + ": " + dependency.second.reason);
        }
    }
    return health;
}

Application::StartupStatus Application::startup() const {
    std::lock_guard lock(stateMutex_);
    return StartupStatus{configValidation_.valid, initialized_, configValidation_.errors};
}

Application::ReadinessStatus Application::readiness() const {
    std::lock_guard lock(stateMutex_);
    if (initialized_ && running_) {
        const auto now = std::chrono::steady_clock::now();
        const auto cacheExpired = !hasProbe_ ||
            now - lastProbeAt_ >= std::chrono::milliseconds(config_.readiness.probeCacheMs);
        if (cacheExpired) {
            refreshDependenciesLocked();
        }
    }

    ReadinessStatus status;
    status.dependencies = {
        {"mysql", dependencies_.mysql},
        {"redis", dependencies_.redis},
        {"mongodb", dependencies_.mongodb},
        {"ai", dependencies_.ai},
    };
    status.ready = initialized_ && running_ && configValidation_.valid;
    for (const auto& dependency : status.dependencies) {
        if (dependency.second.required && !dependency.second.available) {
            status.ready = false;
        }
    }
    return status;
}

void Application::refreshDependenciesLocked() const {
    dependencies_ = data::DataConnectors{config_}.probe();
    lastProbeAt_ = std::chrono::steady_clock::now();
    hasProbe_ = true;
}

api::Router& Application::router() {
    return router_;
}

void Application::registerRoutes() {
    router_.addRoute("GET", "/health", [this] {
        return api::ApiResponse{true, "OK", "服务健康状态已生成", api::toJson(health())};
    });
    router_.addRoute("GET", "/health/live", [this] {
        return api::ApiResponse{isRunning(), isRunning() ? "OK" : "NOT_LIVE", "进程存活状态已生成", "{}"};
    });
    router_.addRoute("GET", "/health/ready", [this] {
        const auto status = readiness();
        return api::ApiResponse{status.ready, status.ready ? "OK" : "NOT_READY", "服务就绪状态已生成", "{}"};
    });
    router_.addRoute("GET", "/health/startup", [this] {
        const auto status = startup();
        return api::ApiResponse{status.initialized && status.configurationValid,
                                status.initialized && status.configurationValid ? "OK" : "STARTUP_FAILED",
                                "服务启动状态已生成", "{}"};
    });
}

}  // namespace induspilot::app
