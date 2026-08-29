# Change: Run the real runtime profile in CI

## Requirement: Cross-platform runtime profile runner

The runtime profile smoke SHALL invoke the HTTP smoke script using an executable available on the current host.

### Scenario: PowerShell Core host runs the profile

- **GIVEN** the profile runner is launched by `pwsh` on a Linux host
- **WHEN** it invokes the HTTP integration smoke
- **THEN** it SHALL invoke PowerShell Core successfully
- **AND** it SHALL preserve the existing environment restoration and dependency cleanup behavior

## Requirement: CI validates the integrated HTTP profile

The CI workflow SHALL build the `dev-http` runtime and run the real dependency profile smoke against MySQL, Redis, and MongoDB.

### Scenario: Integrated profile passes

- **GIVEN** GitHub Actions runs the runtime profile job
- **WHEN** the job starts the Compose dependencies and launches the built backend
- **THEN** dependency CRUD smoke SHALL run
- **AND** HTTP smoke SHALL run with `repository_store=mysql` and `session_store=redis`
- **AND** the job SHALL clean up dependency containers and volumes after success or failure

### Scenario: Existing signals remain independent

- **WHEN** the integrated profile job fails
- **THEN** the foundation build, runtime matrix, and dependency-only job definitions SHALL remain separately executable
