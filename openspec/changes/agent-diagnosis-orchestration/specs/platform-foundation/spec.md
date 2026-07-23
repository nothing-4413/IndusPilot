## MODIFIED Requirements

### Requirement: Runtime configuration
The system SHALL load backend runtime configuration for the C++ service.

#### Scenario: AI provider configuration is loaded
- **WHEN** configuration contains AI provider settings or matching environment overrides
- **THEN** the runtime exposes the selected provider and endpoint to the AI diagnosis module