# Change: CI runtime build matrix

## Requirement: Runtime preset CI coverage

Continuous integration SHALL configure, build, and test the supported Redis and Drogon HTTP runtime presets using their declared dependency manifest.

### Scenario: Redis runtime change is validated

- **WHEN** a pull request changes backend or Redis runtime code
- **THEN** CI configures and builds the `dev-redis` preset with vcpkg
- **AND** CI runs its CTest preset

### Scenario: HTTP runtime change is validated

- **WHEN** a pull request changes backend or HTTP runtime code
- **THEN** CI configures and builds the `dev-http` preset with vcpkg
- **AND** CI runs its CTest preset

### Scenario: Independent matrix diagnostics are retained

- **WHEN** one runtime matrix item fails
- **THEN** the remaining matrix item continues running and reports its own result
