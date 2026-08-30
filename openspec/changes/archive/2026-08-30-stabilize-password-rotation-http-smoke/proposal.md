# Change: Stabilize password rotation HTTP smoke timing

## Why

The HTTP integration smoke configures PBKDF2 with 100,000 iterations but gives password rotation requests the same 10-second timeout as lightweight requests. On slower CI or developer machines, valid password changes can exceed that budget and produce a false smoke failure.

## What Changes

- Give password rotation and the immediately dependent login requests a timeout budget appropriate for the configured password hashing cost.
- Keep the existing short timeout for ordinary HTTP checks.
- Add a testing specification so future credential-cost changes account for smoke timing.

## Impact

- Affected specs: `platform-foundation`
- Affected areas: HTTP integration smoke reliability
