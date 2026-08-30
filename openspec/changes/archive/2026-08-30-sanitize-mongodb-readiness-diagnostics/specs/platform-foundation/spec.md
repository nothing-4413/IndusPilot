# Platform Foundation Diagnostics Delta

## ADDED Requirements

### Requirement: MongoDB readiness diagnostics do not expose credentials

MongoDB readiness SHALL return actionable bounded failure reasons without exposing MongoDB URI userinfo, passwords, or the complete connection URI.

#### Scenario: Driver failure includes URI userinfo
- **GIVEN** a MongoDB driver failure message contains a URI with configured credentials
- **WHEN** readiness reports the authenticated ping failure
- **THEN** the dependency reason SHALL redact the URI userinfo
- **AND** SHALL retain a bounded diagnostic indication of the driver failure

#### Scenario: MongoDB driver emits an oversized diagnostic
- **GIVEN** a MongoDB driver failure message is unusually large
- **WHEN** readiness reports the authenticated ping failure
- **THEN** the dependency reason SHALL remain bounded to the configured diagnostic limit
