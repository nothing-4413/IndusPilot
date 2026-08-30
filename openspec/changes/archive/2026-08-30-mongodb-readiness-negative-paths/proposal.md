# Change: Verify MongoDB readiness negative paths

## Why

The MongoDB-backed HTTP runtime currently exercises only the authenticated success path. A temporary authentication or network failure must keep the process alive so `/health/ready` can report `503`, and the response must not expose connection credentials. Driver-level `ping` response validation also needs regression coverage.

## What Changes

- Allow MongoDB AI storage to defer index reconciliation when the dependency is temporarily unreachable or authentication fails, while retaining startup failure for structural index conflicts.
- Verify that an authenticated account without index-write permission fails closed during index reconciliation without leaking credentials.
- Retry deferred index reconciliation before MongoDB AI reads and writes.
- Add foundation coverage for non-positive MongoDB ping responses and sanitized failure details.
- Extend the real HTTP runtime profile with an incorrect-password readiness smoke and recovery assertion.
- Document the negative-path contract and include the new scenario in CI.

## Non-Goals

- No change to MongoDB schema ownership or index definitions.
- No weakening of the required dependency semantics.
- No logging or returning of MongoDB credentials.
