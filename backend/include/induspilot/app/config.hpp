#pragma once

#include <string>

namespace induspilot::app {

struct DatabaseConfig {
    std::string host{"127.0.0.1"};
    int port{0};
    std::string database;
    std::string user;
    std::string password;
    std::string uri;
};

struct RedisConfig {
    std::string host{"127.0.0.1"};
    int port{6379};
    std::string password;
    int database{0};
    std::string uri{"tcp://127.0.0.1:6379"};
    std::string sessionKeyPrefix{"induspilot:session:"};
    int sessionTtlSeconds{28800};
};

struct AiConfig {
    bool enabled{false};
    std::string endpoint{"http://127.0.0.1:9000"};
};

struct AppConfig {
    std::string host{"0.0.0.0"};
    int port{8080};
    std::string logLevel{"info"};
    DatabaseConfig mysql{"127.0.0.1", 3306, "induspilot", "induspilot", "", ""};
    RedisConfig redis{};
    DatabaseConfig mongodb{"127.0.0.1", 27017, "induspilot", "", "", "mongodb://127.0.0.1:27017"};
    AiConfig ai{};
};

AppConfig loadConfig(const std::string& path);

}  // namespace induspilot::app