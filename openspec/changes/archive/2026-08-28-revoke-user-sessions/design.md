# Design: User-scoped session revocation

## Flow

```text
authenticated password rotation
        |
        v
verify current password and persist new hash
        |
        v
sessionStore.removeForUser(user id)
        |
        +-- success --> audit auth.password.changed, return 200
        |
        +-- failure --> audit auth.password.change.failed, return 503
```

The password hash remains changed if the backing session store becomes unavailable after persistence. This is safer than retaining the old credential, and the failure response tells the caller to authenticate again after storage recovers. Operators can retry the authenticated rotation flow if necessary.

## Session-store contract

`SessionStore::removeForUser` returns true only when the store has completed the revocation operation. A user with no active sessions is successful. The method intentionally does not expose token values.

`InMemorySessionStore` removes matching records while holding its mutex. `RedisSessionStore` maintains a per-user sorted set of session tokens, scored by expiration epoch. Saving a session adds its token to the set and purges stale entries; removal deletes every indexed session key and the index, then scans the session key prefix to revoke sessions written before this index existed. Normal logout removes its token from both locations.

## HTTP and audit contract

`POST /api/v1/auth/password` remains the sole endpoint. On success it returns the existing response envelope. The bearer token used by the request is invalid immediately after the response.

If revocation fails after a password update, the endpoint returns `503` with `SESSION_REVOCATION_FAILED`; the audit event uses `auth.password.change.failed` and result `session_revocation_failed`. Audit events, logs, and responses never include passwords or session tokens.

## Compatibility

Existing Redis session values keep their current serialization. The new user index is populated on future logins, while the prefix scan makes the first revocation after deployment cover old unindexed sessions as well.
