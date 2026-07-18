#include "induspilot/modules/asset_service.hpp"

namespace induspilot::modules {

ServiceStatus AssetService::status() const {
    return ServiceStatus{"industrial-assets", true, "工业资产模块占位就绪"};
}

std::vector<domain::EquipmentAsset> AssetService::listSeedAssets() const {
    return {
        domain::EquipmentAsset{"asset-001", "一号产线主电机", "motor", "示例工厂", "一号车间", "一号产线", domain::AssetStatus::Active},
    };
}

}  // namespace induspilot::modules