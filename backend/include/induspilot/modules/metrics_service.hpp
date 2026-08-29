#pragma once

#include <cstdint>
#include <map>
#include <mutex>
#include <string>

namespace induspilot::modules {

struct HttpMetricSnapshot {
    std::uint64_t count{0};
    std::uint64_t errorCount{0};
    double durationMsSum{0.0};
};

struct ReadinessMetricSnapshot {
    bool ready{false};
    std::uint64_t probeCount{0};
    std::uint64_t failureCount{0};
    std::uint64_t recoveryCount{0};
    std::int64_t lastProbeDurationMs{0};
    std::int64_t lastProbeAtUnixMs{0};
};

struct AiProviderMetricSnapshot {
    std::uint64_t count{0};
    std::uint64_t availableCount{0};
    std::uint64_t unavailableCount{0};
    double durationMsSum{0.0};
};

struct NotificationMetricSnapshot {
    std::uint64_t count{0};
};

std::string normalizeMetricPath(const std::string& path);

class MetricsRegistry {
public:
    void recordHttpRequest(const std::string& method, const std::string& path, int statusCode, double durationMs);
    void recordReadiness(const ReadinessMetricSnapshot& snapshot);
    void recordAiProviderCall(const std::string& provider, const std::string& operation, bool available, double durationMs);
    void recordNotificationDelivery(const std::string& channel, const std::string& outcome);
    std::string renderPrometheus() const;

    std::uint64_t totalRequests() const;
    std::uint64_t totalErrors() const;
    std::uint64_t aiRequests() const;
    std::uint64_t alertClosures() const;
    std::uint64_t workOrderClosures() const;

private:
    static std::string routeKey(const std::string& method, const std::string& path, int statusCode);

    mutable std::mutex mutex_;
    std::map<std::string, HttpMetricSnapshot> httpRoutes_;
    std::map<std::string, AiProviderMetricSnapshot> aiProviderCalls_;
    std::map<std::string, NotificationMetricSnapshot> notificationDeliveries_;
    std::uint64_t totalRequests_{0};
    std::uint64_t totalErrors_{0};
    std::uint64_t aiRequests_{0};
    std::uint64_t alertClosures_{0};
    std::uint64_t workOrderClosures_{0};
    ReadinessMetricSnapshot readiness_;
};

}  // namespace induspilot::modules
