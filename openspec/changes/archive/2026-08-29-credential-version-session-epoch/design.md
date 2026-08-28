# Design: Credential-version session epochs

## Version lifecycle

Every user starts at credential version `1`. A successful password update changes the hash and increments the version in one repository operation. A login copies the current version into the session. Session validation loads the current credential and accepts the session only when the versions match.

```text
password rotation
    | UPDATE hash + version = version + 1
    v
all earlier sessions carry the old version
    |
    v
validateSession rejects them even if storage deletion was incomplete
```

The existing `removeForUser` call remains after persistence. It removes materialized session records promptly, while the epoch check is the durable correctness boundary when deletion is incomplete.

## Storage contract

`UserCredential::credentialVersion` and `SessionInfo::credentialVersion` are unsigned 64-bit values. New records default to `1`. The in-memory repository increments the value under its existing mutex. MySQL adds `users.credential_version` with a migration and updates the hash and version in one SQL statement guarded by the enabled username.

Redis session values use `v2` and append the credential version after the role fields. `v1` values remain parseable, but are assigned version `0`; because current users start at version `1`, an un-migrated session cannot pass identity validation. This avoids silently granting old sessions continued access while allowing the store to read and remove them.

Redis prefix scans skip `user:` sorted-set keys. Indexed tokens are still removed through the sorted set, and the compatibility scan handles unindexed session keys without issuing a string `GET` against an index key.

## Failure semantics

If the repository update succeeds but user-session deletion fails, the existing `SESSION_REVOCATION_FAILED` response remains in force. The password version has already advanced, so all old sessions are rejected by epoch validation once the repository is reachable.
