#pragma once

#include <string>

namespace induspilot::modules {

struct ServiceStatus {
    std::string name;
    bool ready{true};
    std::string message{"模块已就绪"};
};

}  // namespace induspilot::modules