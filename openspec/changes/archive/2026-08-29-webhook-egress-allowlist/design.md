# Design: Webhook egress allowlist

`NotificationConfig` stores an exact, comma-separated host allowlist. Configuration parsing trims entries, and startup validation rejects `webhook_enabled=true` when the list is empty. The sender parses the target URL and compares its host component case-insensitively against the configured entries before creating an HTTP client.

Only exact host matches are accepted; ports remain controlled by the target URL but are not allowed to change the host decision. Rejected targets return a bounded error and use existing notification retry/dead-letter transitions. The allowlist itself is never placed in logs or notification payloads.
