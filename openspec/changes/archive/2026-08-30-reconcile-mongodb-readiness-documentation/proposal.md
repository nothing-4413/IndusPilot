# Change: Reconcile MongoDB readiness documentation

## Why

Several long-lived deployment documents still describe MongoDB as a future-only or TCP-only integration. That conflicts with the current MongoDB AI interaction repository, its startup index reconciliation, bounded metrics, and authenticated readiness probe.

## What Changes

- Correct the production-readiness inventory and priorities.
- Update the technology decision record to describe the current MongoDB AI interaction boundary and authenticated readiness behavior.
- Add a platform specification requirement that keeps operational documentation aligned with implemented dependency behavior.

## Impact

- Affected specs: `platform-foundation`
- Affected areas: production documentation and architecture decision records
