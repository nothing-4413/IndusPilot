## Decision

The logout handler first validates the bearer token. A missing or expired session remains a 401 response. Once validation succeeds, a failed `logout` operation is a storage/revocation failure and returns 503. On success, the handler records an `auth.logout` audit event with the authenticated user id and request trace id; it never records the bearer token.

## Verification

The HTTP smoke logs out an authenticated user, verifies the token is rejected afterward, and confirms an audit event exists without token disclosure.
