## ADDED Requirements

### Requirement: Bounded HTTP provider execution
The system SHALL bound HTTP AI provider retries, total execution time, and response size.

#### Scenario: Transient provider failure is retried within budget
- **WHEN** an HTTP provider returns a transport failure, 408, 429, or 5xx response and retries remain
- **THEN** the provider retries within the configured total timeout and stops after `max_retries` additional attempts

#### Scenario: Oversized provider response is rejected
- **WHEN** a provider response body exceeds `ai.max_response_bytes`
- **THEN** the provider returns an unavailable result and preserves the local diagnosis fallback

### Requirement: AI external data is redacted
The system SHALL redact common credential-style key values before sending AI prompt/context externally and before saving AI interaction inputs.

#### Scenario: Sensitive key value is present
- **WHEN** prompt or context contains a `token`, `password`, `secret`, `api_key`, `authorization`, or `credential` key followed by `:` or `=`
- **THEN** the value is replaced with `[REDACTED]` in the outbound request and AI interaction input
