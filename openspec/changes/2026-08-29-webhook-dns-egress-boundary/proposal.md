# Change: Enforce resolved webhook egress boundaries

## Why

The webhook allowlist currently checks only the configured hostname. A permitted hostname can resolve to loopback, private, link-local, shared, multicast, documentation, or other non-public addresses, allowing an alert rule to probe internal network services.

## What Changes

- Resolve the webhook hostname immediately before creating the HTTP client.
- Reject targets when any resolved address is non-public or unsupported.
- Handle IPv4, IPv6, IPv4-mapped IPv6, and bracketed IPv6 URL hosts safely.
- Keep exact hostname allowlisting and timeout behavior.
- Add regression coverage and document the remaining DNS pinning limitation.

## Non-Goals

- No configurable arbitrary CIDR allowlist.
- No proxy support or network namespace policy.
- No custom TLS certificate trust store.
