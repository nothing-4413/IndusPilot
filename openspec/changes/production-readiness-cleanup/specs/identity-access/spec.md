## ADDED Requirements

### Requirement: Repository-backed identity lookup
The system SHALL authenticate users and resolve role permissions through configured identity repositories.

#### Scenario: User logs in through repository-backed identity
- **WHEN** the identity service receives a login request
- **THEN** it retrieves the user credential from the configured user repository and resolves permissions from the configured permission repository