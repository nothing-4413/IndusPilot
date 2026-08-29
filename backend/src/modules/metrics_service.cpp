#include "induspilot/modules/metrics_service.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <vector>

namespace induspilot::modules {
namespace {

std::vector<std::string> splitPath(const std::string& path) {
    std::vector<std::string> parts;
    std::string current;
    for (const auto ch : path) {
        if (ch == '?') {
            break;
        }
        if (ch == '/') {
            if (!current.empty()) {
                parts.push_back(current);
                current.clear();
            }
            continue;
        }
        current.push_back(ch);
    }
    if (!current.empty()) {
        parts.push_back(current);
    }
    return parts;
}

bool looksLikeIdentifier(const std::string& value) {
    if (value.empty()) {
        return false;
    }
    return std::any_of(value.begin(), value.end(), [](unsigned char ch) {
        return std::isdigit(ch) || ch == '-' || ch == '_';
    });
}

std::string joinPath(const std::vector<std::string>& parts) {
    if (parts.empty()) {
        return "/";
    }
    std::ostringstream out;
    for (const auto& part : parts) {
        out << '/' << part;
    }
    return out.str();
}

std::string escapeLabel(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (const auto ch : value) {
        if (ch == '\\' || ch == '"') {
            escaped.push_back('\\');
        }
        if (ch == '\n') {
            escaped += "\\n";
        } else {
            escaped.push_back(ch);
        }
    }
    return escaped;
}

void appendCounter(std::ostringstream& out, const std::string& name, std::uint64_t value) {
    out << name << ' ' << value << '\n';
}

std::string aiMetricKey(const std::string& provider, const std::string& operation) {
    return provider + '\n' + operation;
}

std::string boundedProvider(const std::string& provider) {
    return provider == "disabled" || provider == "http" ? provider : "unknown";
}

std::string boundedOperation(const std::string& operation) {
    if (operation == "diagnose" || operation == "故障排查" || operation == "日志摘要") {
        return operation;
    }
    return "unknown";
}

}  // namespace

std::string normalizeMetricPath(const std::string& path) {
    auto parts = splitPath(path);
    if (parts.empty()) {
        return "/";
    }
    if (parts.size() >= 4 && parts[0] == "api" && parts[1] == "v1") {
        const auto& resource = parts[2];
        if ((resource == "assets" || resource == "alerts" || resource == "work-orders" || resource == "alert-notifications") && looksLikeIdentifier(parts[3])) {
            parts[3] = "{id}";
        }
    }
    return joinPath(parts);
}

void MetricsRegistry::recordHttpRequest(const std::string& method, const std::string& path, int statusCode, double durationMs) {
    const auto normalizedPath = normalizeMetricPath(path);
    const auto success = statusCode >= 200 && statusCode < 400;
    std::lock_guard<std::mutex> lock(mutex_);
    auto& metric = httpRoutes_[routeKey(method, normalizedPath, statusCode)];
    metric.count += 1;
    metric.durationMsSum += std::max(0.0, durationMs);
    totalRequests_ += 1;
    if (statusCode >= 400) {
        metric.errorCount += 1;
        totalErrors_ += 1;
    }
    if (normalizedPath.rfind("/api/v1/ai/", 0) == 0 && normalizedPath != "/api/v1/ai/status" && normalizedPath != "/api/v1/ai/interactions") {
        aiRequests_ += 1;
    }
    if (success && normalizedPath == "/api/v1/alerts/{id}/close") {
        alertClosures_ += 1;
    }
    if (success && normalizedPath == "/api/v1/work-orders/{id}/close") {
        workOrderClosures_ += 1;
    }
}

void MetricsRegistry::recordReadiness(const ReadinessMetricSnapshot& snapshot) {
    std::lock_guard<std::mutex> lock(mutex_);
    readiness_ = snapshot;
}

void MetricsRegistry::recordAiProviderCall(const std::string& provider, const std::string& operation, bool available, double durationMs) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& metric = aiProviderCalls_[aiMetricKey(boundedProvider(provider), boundedOperation(operation))];
    metric.count += 1;
    if (available) {
        metric.availableCount += 1;
    } else {
        metric.unavailableCount += 1;
    }
    metric.durationMsSum += std::max(0.0, durationMs);
}

std::string MetricsRegistry::renderPrometheus() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream out;
    out << std::fixed << std::setprecision(3);
    out << "# HELP induspilot_http_requests_total Total HTTP requests handled by the backend.\n";
    out << "# TYPE induspilot_http_requests_total counter\n";
    appendCounter(out, "induspilot_http_requests_total", totalRequests_);

    out << "# HELP induspilot_http_errors_total Total HTTP responses with status code >= 400.\n";
    out << "# TYPE induspilot_http_errors_total counter\n";
    appendCounter(out, "induspilot_http_errors_total", totalErrors_);

    out << "# HELP induspilot_ai_requests_total Total AI assistance requests.\n";
    out << "# TYPE induspilot_ai_requests_total counter\n";
    appendCounter(out, "induspilot_ai_requests_total", aiRequests_);

    out << "# HELP induspilot_alert_closures_total Total successful alert close operations.\n";
    out << "# TYPE induspilot_alert_closures_total counter\n";
    appendCounter(out, "induspilot_alert_closures_total", alertClosures_);

    out << "# HELP induspilot_work_order_closures_total Total successful work-order close operations.\n";
    out << "# TYPE induspilot_work_order_closures_total counter\n";
    appendCounter(out, "induspilot_work_order_closures_total", workOrderClosures_);

    out << "# HELP induspilot_readiness Current readiness state (1 ready, 0 not ready).\n";
    out << "# TYPE induspilot_readiness gauge\n";
    out << "induspilot_readiness " << (readiness_.ready ? 1 : 0) << '\n';
    out << "# HELP induspilot_readiness_probes_total Total readiness probes completed.\n";
    out << "# TYPE induspilot_readiness_probes_total counter\n";
    appendCounter(out, "induspilot_readiness_probes_total", readiness_.probeCount);
    out << "# HELP induspilot_readiness_failures_total Total readiness failures.\n";
    out << "# TYPE induspilot_readiness_failures_total counter\n";
    appendCounter(out, "induspilot_readiness_failures_total", readiness_.failureCount);
    out << "# HELP induspilot_readiness_recoveries_total Total readiness recoveries.\n";
    out << "# TYPE induspilot_readiness_recoveries_total counter\n";
    appendCounter(out, "induspilot_readiness_recoveries_total", readiness_.recoveryCount);
    out << "# HELP induspilot_readiness_probe_duration_ms Latest readiness probe duration in milliseconds.\n";
    out << "# TYPE induspilot_readiness_probe_duration_ms gauge\n";
    out << "induspilot_readiness_probe_duration_ms " << readiness_.lastProbeDurationMs << '\n';
    out << "# HELP induspilot_readiness_last_probe_at_unix_ms Unix timestamp of the latest readiness probe.\n";
    out << "# TYPE induspilot_readiness_last_probe_at_unix_ms gauge\n";
    out << "induspilot_readiness_last_probe_at_unix_ms " << readiness_.lastProbeAtUnixMs << '\n';

    out << "# HELP induspilot_ai_provider_calls_total Total AI provider completion calls.\n";
    out << "# TYPE induspilot_ai_provider_calls_total counter\n";
    out << "# HELP induspilot_ai_provider_available_total Total AI provider calls returning available output.\n";
    out << "# TYPE induspilot_ai_provider_available_total counter\n";
    out << "# HELP induspilot_ai_provider_unavailable_total Total AI provider calls returning unavailable output.\n";
    out << "# TYPE induspilot_ai_provider_unavailable_total counter\n";
    out << "# HELP induspilot_ai_provider_duration_ms_sum Total AI provider completion duration in milliseconds.\n";
    out << "# TYPE induspilot_ai_provider_duration_ms_sum counter\n";
    for (const auto& item : aiProviderCalls_) {
        std::istringstream keyStream(item.first);
        std::string provider;
        std::string operation;
        std::getline(keyStream, provider, '\n');
        std::getline(keyStream, operation, '\n');
        const auto labels = std::string("provider=\"") + escapeLabel(provider) + "\",operation=\"" + escapeLabel(operation) + "\"";
        out << "induspilot_ai_provider_calls_total{" << labels << "} " << item.second.count << '\n';
        out << "induspilot_ai_provider_available_total{" << labels << "} " << item.second.availableCount << '\n';
        out << "induspilot_ai_provider_unavailable_total{" << labels << "} " << item.second.unavailableCount << '\n';
        out << "induspilot_ai_provider_duration_ms_sum{" << labels << "} " << item.second.durationMsSum << '\n';
    }

    out << "# HELP induspilot_http_route_requests_total HTTP requests grouped by method, normalized path and status.\n";
    out << "# TYPE induspilot_http_route_requests_total counter\n";
    out << "# HELP induspilot_http_route_duration_ms_sum Total HTTP duration in milliseconds by method, normalized path and status.\n";
    out << "# TYPE induspilot_http_route_duration_ms_sum counter\n";
    out << "# HELP induspilot_http_route_duration_ms_count Total HTTP duration sample count by method, normalized path and status.\n";
    out << "# TYPE induspilot_http_route_duration_ms_count counter\n";
    for (const auto& item : httpRoutes_) {
        std::istringstream keyStream(item.first);
        std::string method;
        std::string normalizedPath;
        std::string status;
        std::getline(keyStream, method, '\n');
        std::getline(keyStream, normalizedPath, '\n');
        std::getline(keyStream, status, '\n');
        const auto labels = std::string("method=\"") + escapeLabel(method) + "\",path=\"" + escapeLabel(normalizedPath) + "\",status=\"" + escapeLabel(status) + "\"";
        out << "induspilot_http_route_requests_total{" << labels << "} " << item.second.count << '\n';
        out << "induspilot_http_route_duration_ms_sum{" << labels << "} " << item.second.durationMsSum << '\n';
        out << "induspilot_http_route_duration_ms_count{" << labels << "} " << item.second.count << '\n';
    }
    return out.str();
}

std::uint64_t MetricsRegistry::totalRequests() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return totalRequests_;
}

std::uint64_t MetricsRegistry::totalErrors() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return totalErrors_;
}

std::uint64_t MetricsRegistry::aiRequests() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return aiRequests_;
}

std::uint64_t MetricsRegistry::alertClosures() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return alertClosures_;
}

std::uint64_t MetricsRegistry::workOrderClosures() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return workOrderClosures_;
}

std::string MetricsRegistry::routeKey(const std::string& method, const std::string& path, int statusCode) {
    return method + '\n' + path + '\n' + std::to_string(statusCode);
}

}  // namespace induspilot::modules
