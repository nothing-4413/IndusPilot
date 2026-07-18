#pragma once

#include <string>

namespace induspilot::app {

enum class LogLevel { Debug, Info, Warn, Error };

class Logger {
public:
    explicit Logger(LogLevel level = LogLevel::Info);
    void info(const std::string& message) const;
    void warn(const std::string& message) const;
    void error(const std::string& message) const;

private:
    LogLevel level_;
};

}  // namespace induspilot::app