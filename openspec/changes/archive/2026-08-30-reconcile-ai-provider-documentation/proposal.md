# Change: Reconcile AI provider documentation

## Why

The AI architecture and local startup documents still describe HTTP provider transport as future work, while the backend already performs bounded authenticated JSON POST requests with retry, response-size, and fallback controls. The architecture document also contains corrupted control characters in configuration names.

## What Changes

- Correct the AI architecture and technology decision records.
- Remove the completed HTTP provider transport from the local-startup backlog.
- Keep remaining work focused on prompt/versioning and provider contract evolution.
- Add a specification requirement for documentation accuracy.

## Impact

- Affected specs: `ai-diagnosis-assistance`
- Affected areas: AI provider documentation and configuration guidance
