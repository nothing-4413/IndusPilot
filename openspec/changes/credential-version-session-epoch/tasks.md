# Tasks

## 1. OpenSpec
- [x] 1.1 Define credential-version session epoch behavior and compatibility rules

## 2. Identity and persistence
- [ ] 2.1 Add credential and session version fields with in-memory repository lifecycle
- [ ] 2.2 Add MySQL column, migration, lookup, and atomic password/version update
- [ ] 2.3 Add Redis v2 serialization, v1 fail-closed compatibility, and safe revocation scan
- [ ] 2.4 Enforce the credential version during login and session validation

## 3. Verification and delivery
- [ ] 3.1 Add regression coverage for version increments, old-session rejection, and Redis compatibility behavior
- [ ] 3.2 Run build, CTest, and applicable HTTP/Redis smoke tests
- [ ] 3.3 Sync the identity specification, update readiness documentation, archive the change, and deliver commits
