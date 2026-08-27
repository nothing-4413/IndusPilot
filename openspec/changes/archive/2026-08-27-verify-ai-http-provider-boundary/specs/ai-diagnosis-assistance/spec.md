## ADDED Requirements

### Requirement: HTTP provider contract is executable
The system SHALL provide a repeatable local integration smoke for the configured HTTP AI provider boundary.

#### Scenario: Provider request contract is preserved
- **WHEN** an enabled HTTP provider processes a diagnosis request
- **THEN** the provider receives a POST request at the configured path with operation, prompt, bounded context items, `X-IndusPilot-Ai-Operation`, and configured authentication header

#### Scenario: Provider success remains auditable
- **WHEN** the provider returns a successful response containing supported text
- **THEN** the diagnosis reports the HTTP provider as available, preserves the provider text, and records the AI interaction

#### Scenario: Provider failure degrades safely
- **WHEN** the provider times out, returns a non-2xx response, or returns unusable content
- **THEN** the diagnosis remains available as a local non-authoritative result, reports the provider as unavailable, and records the AI interaction

### Requirement: Structured diagnosis ownership is explicit
The system SHALL document that the current local orchestration owns structured diagnosis fields even when an HTTP provider returns text.

#### Scenario: External text is returned
- **WHEN** an HTTP provider returns supported text
- **THEN** `AiService` continues to produce risk level, possible causes, recommended actions, and human-review flag locally without treating provider text as an authoritative command
