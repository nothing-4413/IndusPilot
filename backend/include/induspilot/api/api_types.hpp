#pragma once

#include <map>
#include <string>
#include <vector>

namespace induspilot::api {

struct ApiResponse {
    bool success{true};
    std::string code{"OK"};
    std::string message{"操作成功"};
    std::string data{"{}"};
};

struct PageRequest {
    int page{1};
    int pageSize{20};
};

struct HealthCheck {
    std::string service{"induspilot-backend"};
    std::map<std::string, bool> dependencies;
    std::vector<std::string> warnings;
};

std::string toJson(const ApiResponse& response);
std::string toJson(const HealthCheck& health);

}  // namespace induspilot::api