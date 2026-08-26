#pragma once

#include "induspilot/app/config.hpp"

#include <string>

namespace induspilot::data {

struct DependencyRequirements {
    bool mysql{false};
    bool redis{false};
    bool mongodb{false};
    bool ai{false};
    bool aiRequired{false};
};

struct DependencyCheck {
    bool required{false};
    bool available{true};
    std::string reason;
    bool checked{false};
};

struct DependencyStatus {
    DependencyCheck mysql;
    DependencyCheck redis;
    DependencyCheck mongodb;
    DependencyCheck ai;
};

class DataConnectors {
public:
    explicit DataConnectors(app::AppConfig config = app::AppConfig{});

    DependencyRequirements requirements() const;
    DependencyStatus probe() const;
    std::string describe() const;

private:
    app::AppConfig config_;
};

}  // namespace induspilot::data
