#include "induspilot/modules/asset_service.hpp"

namespace induspilot::modules {

AssetService::AssetService() {
    create(domain::EquipmentAsset{"asset-001", "一号产线主电机", "motor", "示例工厂", "一号车间", "一号产线", domain::AssetStatus::Active});
}

ServiceStatus AssetService::status() const {
    return ServiceStatus{"industrial-assets", true, "工业资产模块占位就绪"};
}

domain::EquipmentAsset AssetService::create(domain::EquipmentAsset asset) {
    assets_[asset.id] = asset;
    return asset;
}

std::vector<domain::EquipmentAsset> AssetService::list() const {
    std::vector<domain::EquipmentAsset> result;
    for (const auto& item : assets_) {
        result.push_back(item.second);
    }
    return result;
}

std::optional<domain::EquipmentAsset> AssetService::findById(const std::string& id) const {
    const auto it = assets_.find(id);
    if (it == assets_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool AssetService::updateLifecycleStatus(const std::string& id, domain::AssetStatus status) {
    const auto it = assets_.find(id);
    if (it == assets_.end()) {
        return false;
    }
    it->second.status = status;
    return true;
}

}  // namespace induspilot::modules