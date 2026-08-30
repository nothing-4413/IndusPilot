#include "induspilot/api/api_types.hpp"
#include "induspilot/app/application.hpp"
#include "induspilot/app/config.hpp"

#ifdef INDUSPILOT_WITH_DROGON
#include "induspilot/http/drogon_server.hpp"
#endif

#include <exception>
#include <iostream>

int main(int argc, char** argv) {
    const auto configPath = argc > 1 ? argv[1] : "config/backend.example.yaml";
    auto config = induspilot::app::loadConfig(configPath);

#ifdef INDUSPILOT_WITH_DROGON
    try {
        return induspilot::http::runDrogonServer(config);
    } catch (const std::exception& error) {
        std::cerr << "backend startup failed: " << error.what() << std::endl;
        return 78;
    }
#else
    induspilot::app::Application app(config);
    if (!app.start()) {
        for (const auto& error : app.startup().errors) {
            std::cerr << "invalid configuration: " << error << std::endl;
        }
        return 78;
    }

    std::cout << induspilot::api::toJson(app.health()) << std::endl;
    app.stop();
    return 0;
#endif
}
