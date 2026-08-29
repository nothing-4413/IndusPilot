# Change: Add CI runtime build matrix

## Why

CI currently validates the dependency-free backend build on Linux, but does not compile or test the Redis and Drogon HTTP runtime presets. Regressions in the production runtime boundary can therefore reach the default branch without a matching build signal.

## What Changes

- Add a versioned vcpkg manifest for the Redis and Drogon runtime dependencies.
- Add a Windows GitHub Actions matrix that configures, builds, and tests `dev-redis` and `dev-http` with vcpkg.
- Keep the fast Linux foundation test as a separate signal and clarify the CI coverage in project documentation.

## Non-Goals

- No release artifact publishing or deployment environment rollout.
- No database-dependent HTTP runtime profile execution in the build matrix.
