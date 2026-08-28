# Change: Revoke user sessions after credential rotation

## Why

Password rotation currently leaves every issued session valid until its normal expiry. A stolen token can therefore continue to access protected operational data after the account holder changes their password.

## What Changes

- Add a user-scoped session revocation operation to the session-store boundary.
- Index Redis-backed session tokens by user, with expiry aligned to each session TTL.
- Revoke all of a user's active sessions after a successful password rotation, including the session that submitted the request.
- Return a clear service-unavailable response and audit result if session revocation cannot be completed.
- Cover the in-memory implementation and HTTP contract with regression tests.

## Non-Goals

- No refresh-token protocol, session management UI, or administrator-initiated sign-out.
- No distributed login-failure rate limiting.
- No migration of already expired or externally created Redis keys.
