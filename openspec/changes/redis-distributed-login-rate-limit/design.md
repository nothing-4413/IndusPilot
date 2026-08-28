# Design: Distributed login-failure limiting

## Runtime flow

```text
login request
    |
    v
rateLimiter.check(username)
    | locked -> 429 + Retry-After
    | unavailable -> 503
    v
verify credentials
    | wrong -> rateLimiter.recordFailure(username)
    | correct -> rateLimiter.clear(username)
```

The limiter is checked before credential lookup so locked accounts do not consume password verification work. Redis errors are distinguishable from an unlocked result; when Redis is explicitly configured as the limiter backend, the login request is rejected with `AUTHENTICATION_RATE_LIMITER_UNAVAILABLE` rather than bypassing the control.

## Contract and implementations

`LoginRateLimiter` exposes check, failure recording, and successful-login clearing operations. `InMemoryLoginRateLimiter` stores the existing failure count, first-failure time, and lock deadline under a mutex.

`RedisLoginRateLimiter` uses one failure key and one lock key per username. A Lua script performs `INCR`, initializes the failure-window TTL, and sets the lock TTL when the threshold is reached. This keeps concurrent backend instances from racing between increment and expiration. The lock check uses Redis `PTTL`, and successful login deletes both keys.

Keys are scoped below a configurable prefix and include the username as an opaque Redis key component; passwords and tokens are never stored. The Redis URI is reused from the existing Redis dependency configuration.

## Configuration and compatibility

`security.login_rate_limit_store` accepts `memory` or `redis` and defaults to `memory`, preserving local and existing deployments. Selecting `redis` requires the Redis-enabled build and a usable Redis URI. Redis session storage and login rate limiting are independent choices.

The HTTP login contract keeps `AUTHENTICATION_LOCKED` as `429` with `Retry-After`. A limiter storage failure returns `AUTHENTICATION_RATE_LIMITER_UNAVAILABLE` with `503`; audit records use the existing login failure event without credential or token data.
