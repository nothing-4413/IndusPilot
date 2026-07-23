## ADDED Requirements

### Requirement: Work order persistence repository
The system SHALL persist work order lifecycle records through an injectable work order repository.

#### Scenario: Work order lifecycle is saved through repository
- **WHEN** a work order is created, assigned, started, completed, or closed
- **THEN** the updated work order record is saved through the configured repository