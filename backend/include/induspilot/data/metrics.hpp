#pragma once

#include <string>

namespace induspilot::data {

class AiInteractionMetricsSink {
public:
    virtual ~AiInteractionMetricsSink() = default;

    virtual void recordAiInteractionOperation(
        const std::string& operation,
        bool success,
        double durationMs) = 0;
};

}  // namespace induspilot::data
