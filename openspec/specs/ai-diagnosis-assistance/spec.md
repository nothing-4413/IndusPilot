# ai-diagnosis-assistance Specification

## Purpose
TBD - created by archiving change define-induspilot-foundation. Update Purpose after archive.
## Requirements
### Requirement: AI-assisted alert explanation
The system SHALL provide optional AI-generated explanations for alerts based on alert details, equipment context, and available logs.

#### Scenario: User requests alert explanation
- **WHEN** a user requests AI explanation for an alert
- **THEN** the system returns a suggested explanation or a clear unavailable state without blocking alert handling

### Requirement: AI troubleshooting suggestions
The system SHALL provide troubleshooting suggestions as non-authoritative recommendations.

#### Scenario: Suggestion is generated
- **WHEN** AI returns troubleshooting suggestions
- **THEN** the system labels the content as assistance and requires human judgement before operational action

### Requirement: AI context records
The system SHALL record AI interaction context, inputs, outputs, timestamps, and related business objects for audit and later review.

#### Scenario: AI interaction is completed
- **WHEN** an AI diagnosis request completes
- **THEN** the system stores the interaction record linked to the related alert, asset, or work order when applicable

### Requirement: AI diagnosis HTTP API
The system SHALL expose authenticated HTTP endpoints for requesting non-authoritative AI troubleshooting and log-summary assistance.

#### Scenario: User requests troubleshooting assistance
- **WHEN** a user with `ai:use` submits related business context and a prompt
- **THEN** the system returns an AI assistance response or a clear unavailable response without blocking operations

#### Scenario: User requests log summary assistance
- **WHEN** a user with `ai:use` submits log or context text
- **THEN** the system returns a non-authoritative summary response or a clear unavailable response

### Requirement: AI interaction audit API
The system SHALL expose authenticated HTTP endpoints for reviewing recorded AI interaction audit records.

#### Scenario: User reviews AI interaction history
- **WHEN** a user with `ai:use` requests AI interaction records
- **THEN** the system returns recorded related object, input, and output fields

