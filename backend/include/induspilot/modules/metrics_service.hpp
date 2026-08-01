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

std::string normalizeMetricPath(const std::string& path);

class MetricsRegistry {
public:
    void recordHttpRequest(const std::string& method, const std::string& path, int statusCode, double durationMs);
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
    std::uint64_t totalRequests_{0};
    std::uint64_t totalErrors_{0};
    std::uint64_t aiRequests_{0};
    std::uint64_t alertClosures_{0};
    std::uint64_t workOrderClosures_{0};
};

}  // namespace induspilot::modules