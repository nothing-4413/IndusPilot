# Design: Real runtime profile smoke in CI

The existing `http_runtime_profile_smoke.ps1` remains the single orchestration entry point. It resolves the child PowerShell command from the current platform (`pwsh` when available, otherwise `powershell.exe`) and passes the built backend path through to the existing HTTP smoke script. Dependency startup, CRUD smoke, and cleanup continue to be controlled by the script switches.

GitHub Actions adds an Ubuntu job after the vcpkg-enabled `dev-http` build. The job creates an ephemeral `deployment/.env` with CI-only credentials, starts MySQL, Redis, and MongoDB using Compose, invokes the profile smoke with `-RunDependencySmoke`, and always removes the stack and volumes. The job is separate from the dependency-only job so failures identify whether the issue is dependency CRUD or application integration.

The profile job uses a Linux `dev-http` build because the smoke script is PowerShell Core compatible and Docker services are already native to the Ubuntu runner. It does not share state with other jobs; every job starts from a clean checkout and its own service containers.
