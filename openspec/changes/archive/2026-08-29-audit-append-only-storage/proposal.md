# Change: Enforce append-only operation audit storage

## Why

Operation audit events carry a hash chain, but repository writes currently replace an existing event when the event code is reused. A retry or accidental duplicate can therefore mutate historical actor, resource, timestamp, or hash fields.

## What Changes

- Make in-memory audit persistence keep the first event for a duplicate event code.
- Make MySQL audit persistence use insert-only behavior and return the stored row when a duplicate event code is encountered.
- Add regression coverage proving duplicate writes cannot break the original event or its integrity chain.
- Document the append-only storage boundary.

## Non-Goals

- No new HTTP endpoint for editing or deleting audit events.
- No change to hash calculation, query filters, export shape, or retention scheduling.
- No migration is required because `event_code` is already unique and the existing table remains compatible.
