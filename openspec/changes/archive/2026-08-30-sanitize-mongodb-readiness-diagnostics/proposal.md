# Change: Sanitize MongoDB readiness diagnostics

## Why

MongoDB driver exceptions are currently copied into the readiness dependency reason. URI parsing or connection errors may contain the configured userinfo, and readiness responses are observable by infrastructure and operators outside the backend process.

## What Changes

- Redact MongoDB URI userinfo before exposing driver diagnostics through readiness.
- Bound the diagnostic text length while retaining actionable driver failure context.
- Add regression coverage and document the no-credential-leak contract.

## Impact

- Affected specs: `platform-foundation`
- Affected areas: MongoDB dependency probing and readiness diagnostics
