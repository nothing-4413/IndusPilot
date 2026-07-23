#include "induspilot/modules/audit_service.hpp"

#include "induspilot/data/in_memory_repositories.hpp"

#include <atomic>
#include <chrono>
#include <ctime>
#include <iomanip>
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
    return repository_->save(std::move(event));
}

std::vector<domain::OperationAuditEvent> AuditService::events() const {
    return repository_->list();
}

}  // namespace induspilot::modules