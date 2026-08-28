# Change: Add distributed login-failure limiting

## Why

Login lockout state currently lives inside one `IdentityService` process. With multiple backend instances, an attacker can distribute attempts across instances and avoid the configured account lockout threshold.

## What Changes

- Introduce a login rate-limiter boundary with the existing in-memory behavior as the default.
- Add an atomic Redis implementation that shares failure windows and lock state across instances.
- Make the limiter backend configurable and fail closed when Redis is selected but unavailable.
- Preserve `429` and `Retry-After` for locked accounts and add a `503` response for unavailable security storage.
- Add configuration validation, unit coverage, and Redis smoke coverage.

## Non-Goals

- No IP reputation, CAPTCHA, MFA, or refresh-token changes.
- No changes to password hashing or session expiry.
