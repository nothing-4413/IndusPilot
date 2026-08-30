#pragma once

#include "induspilot/data/repositories.hpp"
#include "induspilot/data/metrics.hpp"

#ifdef INDUSPILOT_WITH_MONGODB
#include <mongocxx/client.hpp>

#include <memory>
#include <string>

namespace induspilot::data {

struct MongoProbeResult {
    bool available{false};
    std::string reason;
};

std::string sanitizeMongoProbeFailure(const std::string& diagnostic);
MongoProbeResult probeMongoDb(const std::string& uri, const std::string& database, int timeoutMs);

class MongoAiInteractionRepository final : public AiInteractionRepository {
public:
    explicit MongoAiInteractionRepository(
        const std::string& uri,
        const std::string& database,
        std::shared_ptr<AiInteractionMetricsSink> metrics = nullptr);

    domain::AiInteraction save(domain::AiInteraction interaction) override;
    std::vector<domain::AiInteraction> list() const override;
    Page list(const Query& query) const override;

private:
    mutable mongocxx::client client_;
    std::string database_;
    std::shared_ptr<AiInteractionMetricsSink> metrics_;
};

}  // namespace induspilot::data
#endif  // INDUSPILOT_WITH_MONGODB
