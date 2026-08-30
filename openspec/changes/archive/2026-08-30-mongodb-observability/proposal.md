# Change: Observe MongoDB AI interaction repository operations

## Why

The MongoDB AI interaction repository is now part of the production persistence path, but `/metrics` cannot distinguish MongoDB read, write, and index-reconciliation failures or latency.

## What Changes

- Add a small data-layer metrics sink that can be implemented by the existing metrics registry.
- Record bounded MongoDB AI interaction operation outcomes and durations.
- Render the repository metrics through the existing Prometheus endpoint.
- Verify the metrics contract in backend tests and document the signals.

## Impact

- Affected specs: `platform-foundation`
- Affected areas: MongoDB repository, metrics registry, HTTP runtime wiring, deployment observability documentation
