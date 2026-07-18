#include "induspilot/api/router.hpp"

#include <utility>

namespace induspilot::api {

void Router::addRoute(std::string method, std::string path, Handler handler) {
    routes_[method + " " + path] = std::move(handler);
}

ApiResponse Router::handle(const std::string& method, const std::string& path) const {
    const auto it = routes_.find(method + " " + path);
    if (it == routes_.end()) {
        return ApiResponse{false, "RESOURCE_NOT_FOUND", "接口不存在", "{}"};
    }
    return it->second();
}

}  // namespace induspilot::api