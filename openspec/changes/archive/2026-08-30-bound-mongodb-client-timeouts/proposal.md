# Change: Bound MongoDB repository client timeouts

## Why

MongoDB readiness probes use a configured timeout, but the MongoDB AI repository client is created from the raw URI. After a dependency failure, the first AI operation can therefore wait for the driver's default server-selection and connect timeouts instead of respecting the runtime's configured dependency budget.

## What Changes

- Pass the configured readiness probe timeout into the MongoDB AI repository.
- Apply the bounded timeout URI to repository client creation and deferred index reconciliation.
- Keep the existing readiness, repository, and error semantics unchanged.

## Non-Goals

- No change to AI provider request timeout or MongoDB operation retry count.
- No change to database schema or index policy.
