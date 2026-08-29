# Change: Constrain webhook egress targets

## Why

Opt-in webhook delivery currently accepts any HTTP(S) target URL. In a production deployment, a compromised or mistaken alert rule could make the backend probe internal services or other unintended hosts.

## What Changes

- Add a configured exact-host allowlist for webhook delivery.
- Require a non-empty allowlist when webhook delivery is enabled.
- Reject webhook targets whose host is not listed, without making an outbound request.
- Keep webhook delivery disabled by default and preserve queue retry/dead-letter behavior.
- Add configuration, smoke, and operator documentation.

## Non-Goals

- No CIDR parser or DNS rebinding protection in this change.
- No automatic discovery of allowed hosts.
- No change to email or console channel behavior.
