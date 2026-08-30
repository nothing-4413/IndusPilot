# AI Diagnosis Assistance Documentation Delta

## ADDED Requirements

### Requirement: AI provider documentation reflects implemented transport

AI provider documentation SHALL describe the implemented bounded HTTP provider transport and SHALL use valid configuration field names.

#### Scenario: Developer enables the HTTP provider
- **WHEN** a developer reads the AI architecture or startup guidance
- **THEN** it SHALL state that Drogon builds POST to the configured `ai.endpoint`
- **AND** it SHALL identify timeout, retry, response-size, authentication, and local fallback behavior

#### Scenario: Developer reads remaining AI work
- **WHEN** a developer reviews future AI work
- **THEN** it SHALL not list the already implemented HTTP transport as pending
