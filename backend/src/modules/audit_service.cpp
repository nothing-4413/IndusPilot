#include "induspilot/modules/audit_service.hpp"

#include "induspilot/data/in_memory_repositories.hpp"
#include "induspilot/modules/password_hasher.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <utility>

namespace induspilot::modules {
namespace {

std::string currentTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &time);
#else
    localtime_r(&time, &localTime);
#endif
    std::ostringstream out;
    out << std::put_time(&localTime, "%Y-%m-%dT%H:%M:%S");
    return out.str();
}

std::string nextAuditId() {
    static std::atomic<unsigned long long> sequence{0};
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const auto epochMillis = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    return "audit-" + std::to_string(epochMillis) + "-" + std::to_string(sequence.fetch_add(1) + 1);
}

std::string canonicalAuditPayload(const domain::OperationAuditEvent& event, const std::string& previousHash) {
    std::ostringstream out;
    out << event.id << '\n'
        << event.actor << '\n'
        << event.action << '\n'
        << event.resourceType << '\n'
        << event.resourceId << '\n'
        << event.result << '\n'
        << event.traceId << '\n'
        << event.occurredAt << '\n'
        << previousHash;
    return out.str();
}

std::string calculateAuditHash(const domain::OperationAuditEvent& event, const std::string& previousHash) {
    return sha256Hex(canonicalAuditPayload(event, previousHash));
}
bool matches(const domain::OperationAuditEvent& event, const OperationAuditQuery& query) {
    return (!query.actor || event.actor == *query.actor) &&
        (!query.action || event.action == *query.action) &&
        (!query.resourceType || event.resourceType == *query.resourceType) &&
        (!query.result || event.result == *query.result);
}

}  // namespace

AuditService::AuditService() : AuditService(std::make_shared<data::InMemoryOperationAuditRepository>()) {}

AuditService::AuditService(std::shared_ptr<data::OperationAuditRepository> repository) : repository_(std::move(repository)) {
    if (!repository_) {
        repository_ = std::make_shared<data::InMemoryOperationAuditRepository>();
    }
}

ServiceStatus AuditService::status() const {
    return ServiceStatus{"operation-audit", true, "operation audit repository is ready"};
}

domain::OperationAuditEvent AuditService::record(domain::OperationAuditEvent event) {
    if (event.id.empty()) {
        event.id = nextAuditId();
    }
    if (event.occurredAt.empty()) {
        event.occurredAt = currentTimestamp();
    }
    if (event.result.empty()) {
        event.result = "success";
    }
    const auto existingEvents = repository_->list();
    event.previousHash = existingEvents.empty() ? "genesis" : existingEvents.front().eventHash;
    event.eventHash = calculateAuditHash(event, event.previousHash);
    return repository_->save(std::move(event));
}

std::vector<domain::OperationAuditEvent> AuditService::events(const OperationAuditQuery& query) const {
    std::vector<domain::OperationAuditEvent> result;
    const auto events = repository_->list();
    std::copy_if(events.begin(), events.end(), std::back_inserter(result), [&query](const auto& event) { return matches(event, query); });
    return result;
}

OperationAuditIntegrityReport AuditService::integrityReport() const {
    auto events = repository_->list();
    std::reverse(events.begin(), events.end());
    OperationAuditIntegrityReport report;
    report.total = events.size();
    std::string previousHash = "genesis";
    for (const auto& event : events) {
        if (event.previousHash != previousHash || event.eventHash != calculateAuditHash(event, event.previousHash)) {
            report.verified = false;
            report.brokenEventId = event.id;
            report.latestHash = previousHash;
            return report;
        }
        previousHash = event.eventHash;
    }
    report.latestHash = previousHash;
    return report;
}
}  // namespace induspilot::modules
