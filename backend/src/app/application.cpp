#include "induspilot/app/application.hpp"

#include "induspilot/api/api_types.hpp"

#include <chrono>
#include <exception>
#include <mutex>
#include <utility>

namespace induspilot::app {

Application::Application(AppConfig config) : config_(std::move(config)) {
    registerRoutes();
}

bool Application::start() {
    {
        std::lock_guard lock(stateMutex_);
        if (running_) {
            return true;
        }
        configValidation_ = validateConfig(config_);
        if (!configValidation_.valid) {
            initialized_ = false;
            running_ = false;
            return false;
        }

        requirements_ = data::DataConnectors{config_}.requirements();
        initialized_ = true;
        running_ = true;
        hasProbe_ = false;
        probeCount_ = 0;
        failureCount_ = 0;
        recoveryCount_ = 0;
        lastProbeDurationMs_ = 0;
        lastProbeAtUnixMs_ = 0;
    }

    refreshDependencies();
    return true;
}

void Application::stop() {
    std::lock_guard lock(stateMutex_);
    running_ = false;
    probeCondition_.notify_all();
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
    refreshDependencies();
    std::lock_guard lock(stateMutex_);
    return readinessLocked();
}

Application::ReadinessStatus Application::readinessLocked() const {
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
    status.probeInProgress = probeInFlight_;
    status.probeCount = probeCount_;
    status.failureCount = failureCount_;
    status.recoveryCount = recoveryCount_;
    status.lastProbeDurationMs = lastProbeDurationMs_;
    status.lastProbeAtUnixMs = lastProbeAtUnixMs_;
    return status;
}

void Application::refreshDependencies() const {
    for (;;) {
        std::unique_lock lock(stateMutex_);
        if (!initialized_ || !running_) {
            return;
        }

        const auto now = std::chrono::steady_clock::now();
        const auto cacheExpired = !hasProbe_ ||
            now - lastProbeAt_ >= std::chrono::milliseconds(config_.readiness.probeCacheMs);
        if (!cacheExpired) {
            return;
        }
        if (probeInFlight_) {
            probeCondition_.wait(lock, [this] {
                return !probeInFlight_ || !initialized_ || !running_;
            });
            continue;
        }

        probeInFlight_ = true;
        const auto config = config_;
        const auto requirements = requirements_;
        lock.unlock();

        const auto startedAt = std::chrono::steady_clock::now();
        data::DependencyStatus nextDependencies;
        try {
            nextDependencies = data::DataConnectors{config}.probe();
        } catch (const std::exception& error) {
            const auto reason = std::string("dependency probe failed: ") + error.what();
            nextDependencies = data::DependencyStatus{
                {requirements.mysql, !requirements.mysql, requirements.mysql ? reason : "not required by repository_store", requirements.mysql},
                {requirements.redis, !requirements.redis, requirements.redis ? reason : "not required by session_store", requirements.redis},
                {false, true, "optional dependency is not probed", false},
                {requirements.aiRequired, !requirements.ai, requirements.ai ? reason : "disabled", requirements.ai},
            };
        } catch (...) {
            const std::string reason = "dependency probe failed: unknown error";
            nextDependencies = data::DependencyStatus{
                {requirements.mysql, !requirements.mysql, requirements.mysql ? reason : "not required by repository_store", requirements.mysql},
                {requirements.redis, !requirements.redis, requirements.redis ? reason : "not required by session_store", requirements.redis},
                {false, true, "optional dependency is not probed", false},
                {requirements.aiRequired, !requirements.ai, requirements.ai ? reason : "disabled", requirements.ai},
            };
        }
        const auto finishedAt = std::chrono::steady_clock::now();
        const auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(finishedAt - startedAt).count();
        const auto unixNow = std::chrono::system_clock::now().time_since_epoch();
        const auto unixMs = std::chrono::duration_cast<std::chrono::milliseconds>(unixNow).count();

        lock.lock();
        if (hasProbe_) {
            const auto updateEdges = [this](const data::DependencyCheck& previous, const data::DependencyCheck& next) {
                if (previous.available && !next.available) {
                    ++failureCount_;
                } else if (!previous.available && next.available) {
                    ++recoveryCount_;
                }
            };
            updateEdges(dependencies_.mysql, nextDependencies.mysql);
            updateEdges(dependencies_.redis, nextDependencies.redis);
            updateEdges(dependencies_.mongodb, nextDependencies.mongodb);
            updateEdges(dependencies_.ai, nextDependencies.ai);
        }
        dependencies_ = std::move(nextDependencies);
        lastProbeAt_ = finishedAt;
        lastProbeDurationMs_ = durationMs;
        lastProbeAtUnixMs_ = unixMs;
        ++probeCount_;
        hasProbe_ = true;
        probeInFlight_ = false;
        lock.unlock();
        probeCondition_.notify_all();
        return;
    }
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
