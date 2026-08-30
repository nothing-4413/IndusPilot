## Why

The in-memory session store leaves expired sessions in its map when they are read, and saving a session with a non-positive TTL retains an immediately expired entry. Redis removes non-positive TTL session keys, so the stores diverge and expired in-memory tokens can accumulate indefinitely.

## What Changes

- Delete expired or inactive in-memory sessions when read.
- Treat a non-positive session TTL as deletion, matching the Redis implementation.
- Verify expired sessions cannot remain removable after cleanup.

## Impact

- Affected spec: `identity-access`
- Affected code: `backend/src/modules/session_store.cpp`
- Affected tests: `backend/tests/backend_foundation_tests.cpp`
