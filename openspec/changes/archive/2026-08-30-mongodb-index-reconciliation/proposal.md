# Change: Reconcile MongoDB AI interaction indexes at startup

## Why

MongoDB initialization scripts run only when a database volume is first created. Existing deployments therefore do not receive the `ai_interactions.interactionCode` unique index introduced after their initial rollout, leaving idempotent writes unenforced.

## What Changes

- Reconcile the required AI interaction indexes when the MongoDB repository is created.
- Fail startup with an operator-actionable error when historical duplicates prevent creation of the unique identity index.
- Exercise the legacy collection-without-index upgrade path in the MongoDB runtime profile.
- Document the production upgrade and duplicate-cleanup procedure.

## Impact

- Affected specs: `platform-foundation`
- Affected areas: MongoDB repository startup, runtime integration smoke, deployment documentation
