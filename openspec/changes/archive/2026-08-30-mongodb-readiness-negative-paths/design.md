# Design: MongoDB readiness negative paths

## Startup behavior

The MongoDB repository probes the configured database with the same authenticated `ping` used by readiness before reconciling indexes. A successful ping performs index reconciliation immediately. A failed ping leaves the repository constructible so the HTTP process can expose liveness and readiness; the repository retries reconciliation on its first read or write. Index reconciliation errors after a successful ping remain startup errors, including duplicate identity data and incompatible indexes.

## HTTP smoke

The runtime profile first runs its normal authenticated MongoDB flow. It then starts the same backend with an intentionally invalid MongoDB password and runs the readiness-only smoke. The smoke requires HTTP `503`, required MongoDB unavailable, a non-empty reason, and no configured MongoDB URI, username, or password in the response. A final normal profile run verifies recovery to HTTP `200` after the cache window is re-evaluated.

The profile also creates a temporary read-only MongoDB user. Because `ping` does not prove index-write permission, this account is expected to fail closed during startup index reconciliation; the process must exit non-zero and stderr must remain credential-free.

## Test boundary

The foundation test exercises the pure `ok` value decision for `0` and negative values. Real MongoDB authentication and HTTP status behavior remain in the Linux CI runtime profile because Docker is not guaranteed on developer workstations.
