# Tasks

- [x] 1. Sanitize and bound MongoDB driver diagnostics returned by readiness.
- [x] 2. Add the readiness diagnostic confidentiality requirement and regression coverage.
- [x] 3. Validate, archive the OpenSpec change, commit, and push.

Validation note: the MongoDB-enabled build and focused backend test passed. The full HTTP integration smoke currently has a separate, reproducible 10-second password-rotation timeout in both `dev-http` and `dev-http-mongodb`; it is tracked for the next change.
