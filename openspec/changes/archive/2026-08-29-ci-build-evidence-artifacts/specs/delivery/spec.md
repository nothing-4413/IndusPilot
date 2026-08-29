# Change: Publish CI build evidence artifacts

## Requirement: CI retains bounded build evidence

The CI workflow SHALL upload backend build evidence after the foundation, runtime matrix, and real runtime profile jobs, including when tests fail.

### Scenario: Foundation evidence is uploaded

- **WHEN** the Linux foundation job completes
- **THEN** CI SHALL publish its backend test executable and CTest temporary log when present
- **AND** the artifact SHALL have a bounded retention period

### Scenario: Runtime matrix evidence is separated

- **WHEN** a Redis or HTTP Windows matrix job completes
- **THEN** CI SHALL publish an artifact named for that matrix entry
- **AND** the artifact SHALL contain the corresponding backend runtime binary and CTest log when present

### Scenario: Failed jobs retain available evidence

- **WHEN** a build or test step fails before all output paths exist
- **THEN** the upload step SHALL still run
- **AND** missing optional files SHALL produce a warning rather than mask the original failure

### Scenario: Sensitive deployment material is excluded

- **THEN** CI artifacts SHALL NOT include `.env` files, Docker volumes, credentials, or source archives
