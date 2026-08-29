# Design: Truthful notification channel delivery

`AlertNotificationSender` is an injectable service boundary returning a delivered flag and a bounded error description. The default sender handles console locally and rejects email. With Drogon enabled, the factory can additionally create a webhook-capable sender when `notifications.webhook_enabled=true`; the webhook target is parsed as an HTTP(S) URL, receives a small JSON payload, and must return 2xx within `notifications.webhook_timeout_ms`.

`AlertService` keeps ownership of attempt counting and queue state. It calls the sender after claiming a record. Sender failure is passed through the existing exponential retry/dead-letter transition, so a disabled or unavailable channel is auditable and never reported as sent.

Webhook transport is opt-in and has a positive timeout bound. The payload contains only notification id, alert id, rule id, message, and attempt count. Authentication headers and provider credentials are intentionally outside this change.
