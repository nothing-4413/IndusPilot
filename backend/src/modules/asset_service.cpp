#include "induspilot/modules/asset_service.hpp"

#include "induspilot/data/in_memory_repositories.hpp"

#include <utility>

namespace induspilot::modules {
namespace {

bool matches(const domain::EquipmentAsset& asset, const AssetQuery& query) {
    if (query.factory && asset.factory != *query.factory) {
        return false;
    }
    if (query.workshop && asset.workshop != *query.workshop) {
        return false;
    }
    if (query.productionLine && asset.productionLine != *query.productionLine) {
        return false;
    }
    if (query.status && asset.status != *query.status) {
        return false;
    }
    return true;
}

}  // namespace

AssetService::AssetService() : AssetService(std::make_shared<data::InMemoryAssetRepository>()) {}

AssetService::AssetService(std::shared_ptr<data::AssetRepository> repository) : repository_(std::move(repository)) {
    if (!repository_) {
        repository_ = std::make_shared<data::InMemoryAssetRepository>();
    }
}

ServiceStatus AssetService::status() const {
    return ServiceStatus{"industrial-assets", true, "asset repository is ready"};
}

domain::EquipmentAsset AssetService::create(domain::EquipmentAsset asset) {
    return repository_->save(std::move(asset));
}

std::vector<domain::EquipmentAsset> AssetService::list(const AssetQuery& query) const {
    std::vector<domain::EquipmentAsset> result;
    for (const auto& asset : repository_->list()) {
        if (matches(asset, query)) {
            result.push_back(asset);
        }
    }
    return result;
}

std::optional<domain::EquipmentAsset> AssetService::findById(const std::string& id) const {
    return repository_->findById(id);
}

bool AssetService::updateLifecycleStatus(const std::string& id, domain::AssetStatus status) {
    auto asset = repository_->findById(id);
    if (!asset) {
        return false;
    }
    asset->status = status;
    repository_->save(*asset);
    return true;
}

}  // namespace induspilot::modules