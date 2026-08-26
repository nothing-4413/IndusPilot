#include "induspilot/http/http_request_lifecycle.hpp"

namespace induspilot::http {

bool HttpRequestLifecycle::tryBeginRequest() {
    std::lock_guard lock(mutex_);
    if (!accepting_) {
        return false;
    }
    ++inFlight_;
    return true;
}

void HttpRequestLifecycle::finishRequest() {
    std::lock_guard lock(mutex_);
    if (inFlight_ == 0) {
        return;
    }
    --inFlight_;
    if (inFlight_ == 0) {
        condition_.notify_all();
    }
}

void HttpRequestLifecycle::stopAccepting() {
    std::lock_guard lock(mutex_);
    accepting_ = false;
    if (inFlight_ == 0) {
        condition_.notify_all();
    }
}

void HttpRequestLifecycle::waitForDrain() const {
    std::unique_lock lock(mutex_);
    condition_.wait(lock, [this] { return inFlight_ == 0; });
}

std::size_t HttpRequestLifecycle::inFlightRequests() const {
    std::lock_guard lock(mutex_);
    return inFlight_;
}

bool HttpRequestLifecycle::accepting() const {
    std::lock_guard lock(mutex_);
    return accepting_;
}

}  // namespace induspilot::http
