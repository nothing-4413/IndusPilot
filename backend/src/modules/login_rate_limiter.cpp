#include "induspilot/modules/login_rate_limiter.hpp"

#ifdef INDUSPILOT_WITH_REDIS
#include <sw/redis++/redis++.h>
#endif

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

namespace induspilot::modules {

int InMemoryLoginRateLimiter::retryAfterSeconds(const FailureState& state, std::chrono::system_clock::time_point now) {
    if (state.lockedUntil <= now) {
        return 0;
    }
    return static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(state.lockedUntil - now).count()) + 1;
}

LoginRateLimitResult InMemoryLoginRateLimiter::check(
    const std::string& username,
    std::chrono::system_clock::time_point now) {
    if (username.empty()) {
        return {};
    }
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = states_.find(username);
    if (it == states_.end()) {
        return {};
    }
    const auto retryAfter = retryAfterSeconds(it->second, now);
    if (retryAfter > 0) {
        return LoginRateLimitResult{true, retryAfter};
    }
    if (it->second.lockedUntil.time_since_epoch().count() > 0) {
        states_.erase(it);
    }
    return {};
}

LoginRateLimitResult InMemoryLoginRateLimiter::recordFailure(
    const std::string& username,
    std::chrono::system_clock::time_point now,
    int maxFailures,
    std::chrono::seconds failureWindow,
    std::chrono::seconds lockDuration) {
    if (username.empty() || maxFailures <= 0) {
        return {};
    }
    std::lock_guard<std::mutex> lock(mutex_);
    auto& state = states_[username];
    if (state.firstFailureAt.time_since_epoch().count() == 0 || now - state.firstFailureAt > failureWindow) {
        state.firstFailureAt = now;
        state.failures = 0;
        state.lockedUntil = {};
    }
    ++state.failures;
    if (state.failures >= maxFailures) {
        state.lockedUntil = now + lockDuration;
        return LoginRateLimitResult{true, retryAfterSeconds(state, now)};
    }
    return {};
}

bool InMemoryLoginRateLimiter::clear(const std::string& username) {
    if (username.empty()) {
        return true;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    states_.erase(username);
    return true;
}

#ifdef INDUSPILOT_WITH_REDIS
struct RedisLoginRateLimiter::Impl {
    explicit Impl(const std::string& uri) : redis(uri) {}

    sw::redis::Redis redis;
};

namespace {

constexpr const char* kRecordFailureScript = R"lua(
local count = redis.call('INCR', KEYS[1])
if count == 1 then
  redis.call('EXPIRE', KEYS[1], ARGV[1])
end
if count >= tonumber(ARGV[2]) then
  redis.call('SET', KEYS[2], '1', 'EX', ARGV[3])
  return ARGV[3]
end
return 0
)lua";

}  // namespace
#endif

RedisLoginRateLimiter::RedisLoginRateLimiter(std::string uri, std::string keyPrefix)
    : keyPrefix_(std::move(keyPrefix)) {
#ifdef INDUSPILOT_WITH_REDIS
    impl_ = std::make_shared<Impl>(std::move(uri));
#else
    (void)uri;
    throw std::runtime_error("Redis login rate limiter is disabled. Enable -DINDUSPILOT_WITH_REDIS=ON and redis-plus-plus.");
#endif
}

LoginRateLimitResult RedisLoginRateLimiter::check(
    const std::string& username,
    std::chrono::system_clock::time_point now) {
#ifdef INDUSPILOT_WITH_REDIS
    (void)now;
    if (username.empty()) {
        return {};
    }
    try {
        const auto ttlMs = impl_->redis.pttl(lockKeyFor(username));
        if (ttlMs <= 0) {
            return {};
        }
        return LoginRateLimitResult{
            true,
            static_cast<int>((ttlMs + 999) / 1000)};
    } catch (const sw::redis::Error&) {
        return LoginRateLimitResult{false, 0};
    }
#else
    (void)username;
    (void)now;
    return LoginRateLimitResult{false, 0};
#endif
}

LoginRateLimitResult RedisLoginRateLimiter::recordFailure(
    const std::string& username,
    std::chrono::system_clock::time_point now,
    int maxFailures,
    std::chrono::seconds failureWindow,
    std::chrono::seconds lockDuration) {
#ifdef INDUSPILOT_WITH_REDIS
    (void)now;
    if (username.empty() || maxFailures <= 0) {
        return {};
    }
    try {
        const auto failureWindowText = std::to_string((std::max)(failureWindow.count(), static_cast<long long>(1)));
        const auto maxFailuresText = std::to_string(maxFailures);
        const auto lockDurationText = std::to_string((std::max)(lockDuration.count(), static_cast<long long>(1)));
        const auto retryAfter = impl_->redis.eval<long long>(
            kRecordFailureScript,
            {keyFor(username), lockKeyFor(username)},
            {failureWindowText, maxFailuresText, lockDurationText});
        return LoginRateLimitResult{true, static_cast<int>(retryAfter)};
    } catch (const sw::redis::Error&) {
        return LoginRateLimitResult{false, 0};
    }
#else
    (void)username;
    (void)now;
    (void)maxFailures;
    (void)failureWindow;
    (void)lockDuration;
    return LoginRateLimitResult{false, 0};
#endif
}

bool RedisLoginRateLimiter::clear(const std::string& username) {
#ifdef INDUSPILOT_WITH_REDIS
    if (username.empty()) {
        return true;
    }
    try {
        const std::vector<std::string> keys{keyFor(username), lockKeyFor(username)};
        impl_->redis.del(keys.begin(), keys.end());
        return true;
    } catch (const sw::redis::Error&) {
        return false;
    }
#else
    (void)username;
    return false;
#endif
}

std::string RedisLoginRateLimiter::keyFor(const std::string& username) const {
    return keyPrefix_ + username;
}

std::string RedisLoginRateLimiter::lockKeyFor(const std::string& username) const {
    return keyPrefix_ + "lock:" + username;
}

std::shared_ptr<LoginRateLimiter> makeRedisLoginRateLimiter(const std::string& uri, const std::string& keyPrefix) {
    return std::make_shared<RedisLoginRateLimiter>(uri, keyPrefix);
}

}  // namespace induspilot::modules
