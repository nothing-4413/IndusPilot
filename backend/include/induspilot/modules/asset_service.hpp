#pragma once

#include "induspilot/domain/domain_types.hpp"
#include "induspilot/modules/service_status.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace induspilot::modules {

class AssetService {
public:
    AssetService();

    ServiceStatus status() const;
    domain::EquipmentAsset create(domain::EquipmentAsset asset);
    std::vector<domain::EquipmentAsset> list() const;
    std::optional<domain::EquipmentAsset> findById(const std::string& id) const;
    bool updateLifecycleStatus(const std::string& id, domain::AssetStatus status);

private:
    std::map<std::string, domain::EquipmentAsset> assets_;
};

}  // namespace induspilot::modules