# Change: Add operation audit time-range filters

## Why

Operation audit queries currently filter by actor, action, resource type, and result, but not by occurrence time. Operators cannot efficiently isolate an incident window, and CSV exports can include unrelated historical records.

## What Changes

- Add inclusive `occurredFrom` and `occurredTo` filters to operation audit queries.
- Validate the timestamp format and reject an inverted time range with `400 INVALID_REQUEST`.
- Apply the same filters to audit CSV export.
- Add service, HTTP smoke, and documentation coverage.

## Non-Goals

- No change to audit event storage or hash-chain calculation.
- No change to existing response shapes or pagination behavior.
- No retention deletion job or database migration in this change.
