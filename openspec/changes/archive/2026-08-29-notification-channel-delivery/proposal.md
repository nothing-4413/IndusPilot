# Change: Make notification channel delivery truthful

## Why

The durable notification queue currently treats `email` and `webhook` records as sent even though no external channel transport exists. This produces false delivery audits and hides operator action when an external notification is not actually delivered.

## What Changes

- Introduce an injectable notification channel sender boundary.
- Keep `console` delivery local and successful.
- Report `email` as unsupported until a real email adapter is added.
- Add an opt-in, timeout-bounded HTTP webhook sender using the notification target URL.
- Route unsupported, disabled, transport-failed, and non-2xx deliveries through existing retry and dead-letter behavior.
- Add configuration, regression coverage, and operator documentation.

## Non-Goals

- No SMTP/email provider implementation.
- No background worker or change to queue lease semantics.
- No credentials in notification metric labels, logs, or webhook payloads.
