# Change: Publish CI build evidence artifacts

## Why

CI currently reports pass/fail status but does not retain the built backend or CTest logs. When a runtime or integration job fails, maintainers must reproduce the run before inspecting the exact test evidence.

## What Changes

- Upload backend runtime binaries and CTest temporary logs from the Linux foundation job.
- Upload each Windows Redis/HTTP matrix build and its CTest logs.
- Upload the Linux real runtime profile build and test evidence.
- Keep uploads available on failed jobs with a bounded retention period and warn when optional paths do not exist.
- Document the artifact names and their intended use.

## Non-Goals

- No release publication, deployment, or signing of binaries.
- No change to build commands, test selection, or artifact contents beyond existing build outputs and logs.
- No upload of credentials, `.env` files, source archives, or dependency volumes.
