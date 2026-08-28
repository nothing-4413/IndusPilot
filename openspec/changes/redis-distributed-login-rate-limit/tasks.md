# Tasks

## 1. OpenSpec
- [x] 1.1 Define distributed limiter, fail-closed, and compatibility behavior

## 2. Limiter implementation
- [ ] 2.1 Extract the in-memory login failure state behind a limiter interface
- [ ] 2.2 Implement atomic Redis failure-window and lock operations
- [ ] 2.3 Add configuration and HTTP status mapping for the selected limiter backend
- [ ] 2.4 Wire the limiter into identity service login flow

## 3. Verification and delivery
- [ ] 3.1 Add local and Redis-backed regression/smoke coverage
- [ ] 3.2 Run build, CTest, and applicable HTTP/Redis smoke tests
- [ ] 3.3 Sync the identity specification, archive the change, and deliver commits
