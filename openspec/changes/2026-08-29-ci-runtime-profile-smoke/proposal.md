# Change: Run the real runtime profile in CI

## Why

CI already builds the Drogon runtime and separately exercises MySQL, Redis, and MongoDB. The runtime profile smoke that combines those dependencies with the real HTTP process is only documented for local use, so configuration or startup regressions at that boundary can pass CI.

## What Changes

- Make the runtime profile smoke invoke the available PowerShell executable on Windows and Linux.
- Add a CI job that builds the HTTP runtime, starts the dependency compose stack, runs dependency CRUD smoke, and runs the MySQL repository plus Redis session HTTP profile.
- Keep the existing fast foundation build, Windows runtime matrix, and dependency-only job as independent signals.
- Document the CI profile coverage and its cleanup behavior.

## Non-Goals

- No change to HTTP API contracts, repository behavior, or dependency images.
- No production deployment or release artifact publishing.
- No requirement to run the Qt client in CI.
