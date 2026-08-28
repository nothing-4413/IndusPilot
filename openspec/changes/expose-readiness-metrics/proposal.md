# Change: Expose readiness metrics

## Why

The readiness endpoint already tracks dependency probe outcomes, but Prometheus only sees the HTTP request that fetched the status. Operators cannot alert on readiness loss, probe failures, or recovery without scraping JSON and interpreting application fields.

## What Changes

- Record readiness state and probe outcome counters in the existing metrics registry.
- Export a readiness gauge, probe/failure/recovery counters, and the latest probe duration and timestamp in Prometheus format.
- Update the readiness route, unit coverage, smoke assertions, and observability documentation.

## Non-Goals

- No new metrics endpoint or labels containing dependency names.
- No change to readiness decision semantics or probe scheduling.
