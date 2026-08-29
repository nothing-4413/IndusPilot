# Change: Govern MySQL migration execution

## Why

The repository has ordered, idempotent MySQL scripts, but deployment currently replays every SQL file directly. That makes it easy to miss a migration, apply an unknown version, or continue after a partially ordered schema state without a clear failure.

## What Changes

- Add one MySQL migration runner that applies scripts in numeric order.
- Refuse unknown recorded versions, missing earlier versions, and scripts that do not register their expected version.
- Make the dependency smoke use the same runner instead of duplicating execution logic.
- Extend deployment preflight and operator documentation with the controlled migration entrypoint.

## Non-Goals

- No automatic rollback or destructive schema changes.
- No change to application startup or automatic database mutation.
- No replacement of the existing SQL migrations with a third-party framework.
