#pragma once

#include <string>

namespace induspilot::data {

struct DependencyStatus {
    bool mysql{false};
    bool redis{false};
    bool mongodb{false};
    bool ai{false};
};

class DataConnectors {
public:
    DependencyStatus probe() const;
    std::string describe() const;
};

}  // namespace induspilot::data