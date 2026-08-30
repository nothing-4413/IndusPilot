# Change: Use authenticated MongoDB readiness checks

## Why

MongoDB readiness currently checks only whether its TCP port is reachable. A live port does not prove that configured credentials, database access, or MongoDB command execution are usable by the selected AI interaction repository.

## What Changes

- Add a bounded authenticated MongoDB `ping` probe when MongoDB AI storage is required.
- Preserve the existing TCP fallback for builds without the MongoDB adapter.
- Expose actionable authentication and command failure reasons through readiness.
- Update readiness specifications and deployment documentation.

## Impact

- Affected specs: `platform-foundation`
- Affected areas: dependency probing, MongoDB adapter, readiness documentation
