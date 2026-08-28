#pragma once

#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace induspilot::modules {

struct LoginRateLimitResult {
    bool available{true};
    int retryAfterSeconds{0};
};

class LoginRateLimiter {
public:
    virtual ~LoginRateLimiter() = default;

    virtual LoginRateLimitResult check(const std::string& username, std::chrono::system_clock::time_point now) = 0;
    virtual LoginRateLimitResult recordFailure(
        const std::string& username,
        std::chrono::system_clock::time_point now,
        int maxFailures,
        std::chrono::seconds failureWindow,
        std::chrono::seconds lockDuration) = 0;
    virtual bool clear(const std::string& username) = 0;
};

class InMemoryLoginRateLimiter final : public LoginRateLimiter {
public:
    LoginRateLimitResult check(const std::string& username, std::chrono::system_clock::time_point now) override;
    LoginRateLimitResult recordFailure(
        const std::string& username,
        std::chrono::system_clock::time_point now,
        int maxFailures,
        std::chrono::seconds failureWindow,
        std::chrono::seconds lockDuration) override;
    bool clear(const std::string& username) override;

private:
    struct FailureState {
        int failures{0};
        std::chrono::system_clock::time_point firstFailureAt{};
        std::chrono::system_clock::time_point lockedUntil{};
    };

    static int retryAfterSeconds(const FailureState& state, std::chrono::system_clock::time_point now);

    std::mutex mutex_;
    std::unordered_map<std::string, FailureState> states_;
};

class RedisLoginRateLimiter final : public LoginRateLimiter {
public:
    explicit RedisLoginRateLimiter(std::string uri, std::string keyPrefix = "induspilot:login-limit:");

    LoginRateLimitResult check(const std::string& username, std::chrono::system_clock::time_point now) override;
    LoginRateLimitResult recordFailure(
        const std::string& username,
        std::chrono::system_clock::time_point now,
        int maxFailures,
        std::chrono::seconds failureWindow,
        std::chrono::seconds lockDuration) override;
    bool clear(const std::string& username) override;

private:
    struct Impl;

    std::string keyFor(const std::string& username) const;
    std::string lockKeyFor(const std::string& username) const;

    std::shared_ptr<Impl> impl_;
    std::string keyPrefix_;
};

std::shared_ptr<LoginRateLimiter> makeRedisLoginRateLimiter(
    const std::string& uri,
    const std::string& keyPrefix = "induspilot:login-limit:");

}  // namespace induspilot::modules
