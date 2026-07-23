## ADDED Requirements

### Requirement: Runtime state persistence repository
The system SHALL persist latest runtime states through an injectable runtime state repository.

#### Scenario: Runtime state is saved through repository
- **WHEN** a runtime state is written for an asset
- **THEN** the latest state is saved through the configured repository and can be queried after the write