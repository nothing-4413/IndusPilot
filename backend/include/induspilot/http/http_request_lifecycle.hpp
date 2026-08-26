#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>

namespace induspilot::http {

class HttpRequestLifecycle {
public:
    bool tryBeginRequest();
    void finishRequest();
    void stopAccepting();
    void waitForDrain() const;
    std::size_t inFlightRequests() const;
    bool accepting() const;

private:
    mutable std::mutex mutex_;
    mutable std::condition_variable condition_;
    bool accepting_{true};
    std::size_t inFlight_{0};
};

}  // namespace induspilot::http
