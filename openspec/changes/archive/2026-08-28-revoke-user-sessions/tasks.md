# Tasks

## 1. OpenSpec
- [x] 1.1 Define user-scoped session revocation behavior and failure semantics

## 2. Session storage
- [x] 2.1 Extend the session-store interface and in-memory implementation with user-scoped removal
- [x] 2.2 Add a TTL-aware Redis user-session index and keep it consistent on save and logout

## 3. Identity and HTTP
- [x] 3.1 Revoke all user sessions after a successful password hash update
- [x] 3.2 Map revocation failures to the HTTP and audit contracts

## 4. Verification and delivery
- [x] 4.1 Add service and HTTP regression coverage for token invalidation and re-login
- [x] 4.2 Run relevant CMake and HTTP quality gates
- [x] 4.3 Sync the identity specification and archive the change
