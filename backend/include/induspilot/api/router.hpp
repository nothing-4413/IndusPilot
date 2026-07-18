#pragma once

#include "induspilot/api/api_types.hpp"

#include <functional>
#include <map>
#include <string>

namespace induspilot::api {

class Router {
public:
    using Handler = std::function<ApiResponse()>;

    void addRoute(std::string method, std::string path, Handler handler);
    ApiResponse handle(const std::string& method, const std::string& path) const;

private:
    std::map<std::string, Handler> routes_;
};

}  // namespace induspilot::api