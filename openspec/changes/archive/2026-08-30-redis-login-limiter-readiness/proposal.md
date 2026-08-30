# Change: Require Redis readiness for distributed login limiting

## Why

Redis is currently considered a required readiness dependency only when Redis-backed sessions are selected. A deployment that keeps sessions in memory but selects the Redis distributed login limiter can report ready even though authentication will fail closed with `503` because its security storage is unavailable.

## What Changes

- Treat Redis as required whenever either Redis session storage or Redis login-failure limiting is configured.
- Preserve the existing probe implementation and dependency response shape.
- Add regression coverage for limiter-only Redis readiness and document the dependency rule.

## Non-Goals

- No change to session storage semantics, login lockout rules, or Redis key formats.
- No additional dependency type or readiness endpoint.
