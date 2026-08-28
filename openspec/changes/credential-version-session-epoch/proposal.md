# Change: Enforce credential-version session epochs

## Why

Session deletion is best-effort at the storage boundary. If Redis is temporarily unavailable or an old session key is not indexed, a bearer token could remain usable after a password rotation. A credential version gives the identity service a durable, user-scoped revocation boundary that is checked on every session validation.

## What Changes

- Add a monotonically increasing credential version to user credentials and issued sessions.
- Atomically update the password hash and credential version during password rotation.
- Reject sessions whose signed credential version differs from the current repository value.
- Persist the version in MySQL and use a versioned Redis session format with legacy parsing behavior.
- Fix Redis user-session revocation scans so sorted-set index keys are never read as string values.
- Cover version changes, legacy session compatibility, and revocation behavior with regression tests.

## Non-Goals

- No refresh-token protocol or token introspection endpoint.
- No change to the existing session TTL or bearer-token response shape.
- No distributed login-failure rate limiting in this change.
