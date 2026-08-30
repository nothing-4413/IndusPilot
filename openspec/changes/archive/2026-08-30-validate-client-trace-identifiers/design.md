## Context

Trace identifiers cross a trust boundary: they originate in HTTP request headers but are used in response headers, structured logs, and audit records. The generated trace format is already stable and suitable as a fallback.

## Goals / Non-Goals

### Goals

- Ensure externally supplied trace identifiers are safe to put in response headers and persisted observability records.
- Keep client correlation working for conventional opaque identifiers.
- Make invalid input fall back deterministically without rejecting the business request.

### Non-Goals

- Adopt a distributed tracing protocol or change the generated trace identifier format.
- Change audit retention or request logging fields.

## Decisions

### Validate the selected incoming header before reuse

The runtime will select `X-Trace-Id` first, then `X-Request-Id` only when the former is absent. It will reuse the selected value only when it contains 1-128 printable ASCII characters excluding whitespace and HTTP control characters. Otherwise it generates the existing server trace identifier.

This prevents oversized and non-header-safe values from reaching every trace sink while preserving normal identifiers such as UUIDs, request IDs, and W3C-style opaque tokens.

### Do not fall through from an invalid X-Trace-Id to X-Request-Id

Header precedence remains deterministic. When a supplied `X-Trace-Id` is present but invalid, generating a fresh identifier is safer than silently allowing a second client-controlled identifier to take precedence.

## Risks / Trade-offs

- Clients using spaces or non-ASCII trace IDs will receive a new trace ID. Such identifiers are unsuitable for portable HTTP-header propagation and should migrate to opaque ASCII tokens.
- The 128-character cap intentionally rejects unusual long identifiers to bound header, log, and audit storage.

## Verification

- Extend the HTTP common unit test to prove valid values propagate unchanged.
- Verify whitespace, control characters, non-ASCII bytes, and oversized values generate a new `trace-<timestamp>-<sequence>` identifier.
- Run the relevant HTTP test profile and quality gates.
