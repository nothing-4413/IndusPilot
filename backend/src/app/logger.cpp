#include "induspilot/app/logger.hpp"

#include <iostream>

namespace induspilot::app {

Logger::Logger(LogLevel level) : level_(level) {}

void Logger::info(const std::string& message) const {
    if (level_ <= LogLevel::Info) {
        std::cout << "[INFO] " << message << std::endl;
    }
}

void Logger::warn(const std::string& message) const {
    if (level_ <= LogLevel::Warn) {
        std::cout << "[WARN] " << message << std::endl;
    }
}

void Logger::error(const std::string& message) const {
    std::cerr << "[ERROR] " << message << std::endl;
}

}  // namespace induspilot::app