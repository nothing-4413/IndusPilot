# Change: Add durable notification delivery queue

## Why

Alert notification dispatch currently lists every notification and updates each eligible row synchronously. Multiple backend instances can select the same row, transient failures have no scheduled backoff, and permanently invalid notifications remain retryable forever.

## What Changes

- Add durable delivery scheduling and short-lived worker leases to notification records.
- Atomically claim due notifications so concurrent workers do not deliver the same record.
- Apply bounded exponential retry backoff and move exhausted records to `dead_letter`.
- Preserve the existing HTTP dispatch and retry endpoints while exposing queue state and attempt metadata.
- Add migration, repository, service, smoke, and documentation coverage.

## Non-Goals

- No external email/webhook provider implementation.
- No background thread or distributed scheduler; dispatch remains explicitly triggered by the existing endpoint.
