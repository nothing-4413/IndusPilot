#include "induspilot/api/api_types.hpp"
#include "induspilot/app/application.hpp"
#include "induspilot/app/config.hpp"

#include <iostream>

int main(int argc, char** argv) {
    const auto configPath = argc > 1 ? argv[1] : "config/backend.example.yaml";
    auto config = induspilot::app::loadConfig(configPath);
    induspilot::app::Application app(config);
    app.start();

    std::cout << induspilot::api::toJson(app.health()) << std::endl;
    app.stop();
    return 0;
}