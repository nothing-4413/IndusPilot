# Design: Redis readiness for authentication controls

`DataConnectors::requirements()` is the single source of required-dependency selection for application health and readiness. Its Redis requirement will become the logical OR of `redis.session_store=redis` and `security.login_rate_limit_store=redis`.

This makes a limiter-only deployment execute the existing bounded Redis TCP probe. A failed probe retains the established behavior: `/health/live` remains available, while `/health/ready` reports Redis as required and unavailable with HTTP `503`. No Redis credential material is used in the TCP probe or emitted by its stable diagnostics.

The foundation tests cover the selection rule and its unavailable result using a loopback endpoint with no listener. Documentation describes both authentication consumers so operators configure readiness expectations correctly.
