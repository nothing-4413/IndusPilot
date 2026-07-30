## ADDED Requirements

### Requirement: Configurable AI provider boundary
The system SHALL support a configurable AI provider boundary for diagnosis assistance.

#### Scenario: Disabled provider remains usable
- **WHEN** the configured AI provider is `disabled`
- **THEN** the system returns a deterministic fallback diagnosis and records the interaction audit

#### Scenario: HTTP provider is configured
- **WHEN** the configured AI provider is `http`
- **THEN** the system reports the configured endpoint in AI status and keeps the diagnosis flow behind the same provider interface

### Requirement: Agent diagnosis orchestration
The system SHALL orchestrate industrial diagnosis requests into structured, auditable outputs.

#### Scenario: Diagnosis request contains industrial context
- **WHEN** a diagnosis request includes related asset, alert, runtime state, work-order history, and operator description
- **THEN** the system returns a diagnosis result with summary, possible causes, recommended actions, risk level, and human-review flag

#### Scenario: Diagnosis request is audited
- **WHEN** a diagnosis request is processed
- **THEN** the request context and generated diagnosis output are saved through the AI interaction repository