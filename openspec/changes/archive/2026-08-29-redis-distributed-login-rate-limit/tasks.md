# Tasks

## 1. OpenSpec
- [x] 1.1 Define distributed limiter, fail-closed, and compatibility behavior

## 2. Limiter implementation
- [x] 2.1 Extract the in-memory login failure state behind a limiter interface
- [x] 2.2 Implement atomic Redis failure-window and lock operations
- [x] 2.3 Add configuration and HTTP status mapping for the selected limiter backend
- [x] 2.4 Wire the limiter into identity service login flow

## 3. Verification and delivery
- [x] 3.1 Add local and Redis-backed regression/smoke coverage
- [x] 3.2 Run build, CTest, and applicable HTTP/Redis smoke tests
- [x] 3.3 Sync the identity specification, archive the change, and deliver commits
