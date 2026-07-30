## ADDED Requirements

### Requirement: Qt client AI diagnosis synchronization
The system SHALL allow the Qt client to submit structured AI diagnosis context to the backend after login.

#### Scenario: Diagnosis request is submitted from Qt client
- **WHEN** an authenticated user fills diagnosis context and runs AI diagnosis
- **THEN** the client calls the backend AI diagnosis endpoint with related object, prompt, and context fields

### Requirement: Qt client AI diagnosis presentation
The system SHALL present backend AI diagnosis results in a structured operator-readable format.

#### Scenario: Diagnosis result is returned
- **WHEN** the backend returns summary, risk level, possible causes, recommended actions, and review metadata
- **THEN** the Qt client displays those fields without requiring the operator to inspect raw JSON

### Requirement: Qt client AI diagnosis fallback
The system SHALL keep AI diagnosis non-blocking when backend or provider calls are unavailable.

#### Scenario: Diagnosis request cannot be completed online
- **WHEN** the Qt client cannot obtain an online AI diagnosis result
- **THEN** it shows a local fallback message and keeps core alert and work-order workflows available