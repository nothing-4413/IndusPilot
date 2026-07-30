## ADDED Requirements

### Requirement: Qt client AI interaction audit synchronization
The system SHALL allow the Qt client to load AI interaction audit records from the backend after login.

#### Scenario: AI interactions are loaded for a related object
- **WHEN** an authenticated user refreshes AI interaction history for a related type and related id
- **THEN** the client calls the backend AI interactions endpoint and displays matching audit rows

### Requirement: Qt client AI interaction audit refresh
The system SHALL refresh AI interaction audit rows after a new AI diagnosis is submitted.

#### Scenario: Diagnosis creates an audit record
- **WHEN** the Qt client receives a diagnosis response
- **THEN** it refreshes the AI interaction history table for the current related object

### Requirement: Qt client AI interaction audit fallback
The system SHALL keep local AI interaction demo rows available when backend audit calls fail.

#### Scenario: Backend AI interaction request fails
- **WHEN** the Qt client cannot load AI interaction history
- **THEN** it keeps local demo rows visible and shows an offline fallback message