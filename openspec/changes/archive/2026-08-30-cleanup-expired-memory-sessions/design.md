## Decision

`InMemorySessionStore::find` already holds the store mutex, so it can erase an expired or inactive entry before returning no session. `save` will erase and succeed for non-positive TTL instead of retaining a dead entry. This matches Redis `save` behavior and bounds retained state without changing valid session behavior.

## Verification

The foundation test saves a session with zero TTL, confirms it cannot be found, and confirms no stale entry remains removable.
