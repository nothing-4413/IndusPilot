#pragma once

#include "induspilot/data/repositories.hpp"

#ifdef INDUSPILOT_WITH_MONGODB
#include <mongocxx/client.hpp>

#include <memory>
#include <string>

namespace induspilot::data {

class MongoAiInteractionRepository final : public AiInteractionRepository {
public:
    explicit MongoAiInteractionRepository(const std::string& uri, const std::string& database);

    domain::AiInteraction save(domain::AiInteraction interaction) override;
    std::vector<domain::AiInteraction> list() const override;
    Page list(const Query& query) const override;

private:
    mutable mongocxx::client client_;
    std::string database_;
};

}  // namespace induspilot::data
#endif  // INDUSPILOT_WITH_MONGODB
