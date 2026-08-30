#include "induspilot/app/config.hpp"

#include <cassert>

int main() {
    auto redisSessionConfig = induspilot::app::AppConfig{};
    redisSessionConfig.redis.sessionStore = "redis";

    auto redisLimiterConfig = induspilot::app::AppConfig{};
    redisLimiterConfig.security.loginRateLimitStore = "redis";

#ifdef INDUSPILOT_WITH_REDIS
    assert(induspilot::app::validateConfig(redisSessionConfig).valid);
    assert(induspilot::app::validateConfig(redisLimiterConfig).valid);
#else
    assert(!induspilot::app::validateConfig(redisSessionConfig).valid);
    assert(!induspilot::app::validateConfig(redisLimiterConfig).valid);
#endif

    assert(induspilot::app::validateConfig(induspilot::app::AppConfig{}).valid);
    return 0;
}
