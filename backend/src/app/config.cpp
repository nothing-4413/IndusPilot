#include "induspilot/app/config.hpp"

#include <fstream>
#include <string>

namespace induspilot::app {

namespace {
std::string trim(const std::string& value) {
    const auto begin = value.find_first_not_of(" \t\r\n\"");
    const auto end = value.find_last_not_of(" \t\r\n\"");
    if (begin == std::string::npos || end == std::string::npos) {
        return {};
    }
    return value.substr(begin, end - begin + 1);
}
}  // namespace

AppConfig loadConfig(const std::string& path) {
    AppConfig config;
    std::ifstream input(path);
    if (!input) {
        return config;
    }

    // 轻量解析仅用于开发骨架，后续应替换为正式 YAML/JSON 配置库。
    std::string section;
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        if (line.find(':') != std::string::npos && line.find("  ") != 0) {
            section = trim(line.substr(0, line.find(':')));
            continue;
        }
        const auto pos = line.find(':');
        if (pos == std::string::npos) {
            continue;
        }
        const auto key = trim(line.substr(0, pos));
        const auto value = trim(line.substr(pos + 1));
        if (section == "server" && key == "host") {
            config.host = value;
        } else if (section == "server" && key == "port") {
            config.port = std::stoi(value);
        } else if (section == "log" && key == "level") {
            config.logLevel = value;
        } else if (section == "ai" && key == "enabled") {
            config.ai.enabled = value == "true";
        } else if (section == "ai" && key == "endpoint") {
            config.ai.endpoint = value;
        }
    }

    return config;
}

}  // namespace induspilot::app