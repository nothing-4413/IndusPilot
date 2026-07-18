#pragma once

#include <string>

namespace induspilot::app {

struct DatabaseConfig {
    std::string host{"127.0.0.1"};
    int port{0};
    std::string database;
    std::string user;
};

struct AiConfig {
    bool enabled{false};
    std::string endpoint{"http://127.0.0.1:9000"};
};

struct AppConfig {
    std::string host{"0.0.0.0"};
    int port{8080};
    std::string logLevel{"info"};
    DatabaseConfig mysql{"127.0.0.1", 3306, "induspilot", "induspilot"};
    DatabaseConfig redis{"127.0.0.1", 6379, "", ""};
    DatabaseConfig mongodb{"127.0.0.1", 27017, "induspilot", ""};
    AiConfig ai{};
};

AppConfig loadConfig(const std::string& path);

}  // namespace induspilot::app