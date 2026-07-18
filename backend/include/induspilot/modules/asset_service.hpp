#pragma once

#include "induspilot/data/repositories.hpp"
#include "induspilot/domain/domain_types.hpp"
#include "induspilot/modules/service_status.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace induspilot::modules {

struct AssetQuery {
    std::optional<std::string> factory;
    std::optional<std::string> workshop;
    std::optional<std::string> productionLine;
    std::optional<domain::AssetStatus> status;
};

class AssetService {
public:
    AssetService();
    explicit AssetService(std::shared_ptr<data::AssetRepository> repository);

    ServiceStatus status() const;
    domain::EquipmentAsset create(domain::EquipmentAsset asset);
    std::vector<domain::EquipmentAsset> list(const AssetQuery& query = {}) const;
    std::optional<domain::EquipmentAsset> findById(const std::string& id) const;
    bool updateLifecycleStatus(const std::string& id, domain::AssetStatus status);

private:
    std::shared_ptr<data::AssetRepository> repository_;
};

}  // namespace induspilot::modules