## Why

The HTTP runtime currently reflects arbitrary `X-Trace-Id` and `X-Request-Id` values into response headers, request logs, and audit records. Unbounded or control-character-bearing values do not form a safe correlation identifier and can create inconsistent proxy, logging, and audit behavior.

## What Changes

- Accept client-supplied trace identifiers only when they are short visible ASCII tokens.
- Generate a server trace identifier when both supplied headers are absent or invalid.
- Preserve the existing precedence of `X-Trace-Id` over `X-Request-Id` when the selected value is valid.
- Cover valid propagation and invalid-value fallback in HTTP common tests and document the contract.

## Impact

- Affected spec: `platform-foundation`
- Affected code: `backend/src/http/http_common.cpp`, `backend/tests/http_common_tests.cpp`
- Affected documentation: `docs/development/backend-http-service.md`
