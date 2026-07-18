#pragma once

#include "induspilot/app/config.hpp"

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
    explicit DataConnectors(app::AppConfig config = app::AppConfig{});

    DependencyStatus probe() const;
    std::string describe() const;

private:
    app::AppConfig config_;
};

}  // namespace induspilot::data