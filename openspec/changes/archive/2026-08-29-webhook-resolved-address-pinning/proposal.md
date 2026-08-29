# Change: Pin webhook connections to validated addresses

## Why

Webhook delivery currently validates DNS results and then gives the hostname back to the HTTP client. A resolver change between validation and connect can therefore redirect the request to a private address.

## What Changes

- Return a validated public address from webhook resolution instead of a boolean.
- Create the Drogon client with that address and port, while preserving the HTTP Host header.
- Keep TLS certificate validation enabled; HTTPS targets that require hostname SNI must use a connector that supports explicit SNI rather than silently bypassing validation.
- Add regression coverage and update the security documentation.

## Non-Goals

- No disabling of TLS certificate validation.
- No global DNS cache or hosts-file mutation.
