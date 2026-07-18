#pragma once

#include "induspilot/domain/domain_types.hpp"
#include "induspilot/modules/service_status.hpp"

#include <vector>

namespace induspilot::modules {

class AssetService {
public:
    ServiceStatus status() const;
    std::vector<domain::EquipmentAsset> listSeedAssets() const;
};

}  // namespace induspilot::modules