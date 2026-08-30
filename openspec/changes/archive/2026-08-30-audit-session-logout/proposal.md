## Why

Successful logout is a security-relevant session revocation but is not auditable. After an otherwise valid session is checked, a session-store deletion failure is also indistinguishable from an invalid credential to callers.

## What Changes

- Validate a logout session before revocation to retain the authenticated actor and trace id.
- Record successful logout as `auth.logout` without persisting the session token.
- Return HTTP 503 for a revocation failure after successful validation.

## Impact

- Affected specs: `identity-access`, `platform-foundation`
- Affected code: `backend/src/http/routes/auth_routes.cpp`
- Affected verification: HTTP integration smoke
