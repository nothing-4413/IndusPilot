# Change: Reject Redis selections unavailable in the build

## Why

Selecting `redis.session_store=redis` in a binary built without Redis support currently falls back silently to in-memory sessions. The HTTP runtime also builds its service context before static configuration validation, allowing invalid feature selections to reach construction code.

## What Changes

- Reject Redis session storage and distributed login-rate limiting when the binary lacks `INDUSPILOT_WITH_REDIS`.
- Validate static configuration before building the HTTP service context.
- Add a defensive composition-root error for an unavailable Redis session store selection.
- Cover no-Redis and Redis-enabled configuration profiles and document the build requirement.

## Non-Goals

- No change to Redis connectivity readiness semantics.
- No automatic dependency installation or runtime fallback between storage types.
