# Design: CI runtime build matrix

The repository root contains `vcpkg.json`, declaring `redis-plus-plus` and the Drogon feature set required by the existing `dev-redis` and `dev-http` presets. This turns implicit workstation packages into an explicit CI input.

`backend-runtime-matrix` runs on `windows-latest` because the HTTP CTest registrations use Windows PowerShell. Each matrix entry invokes the matching CMake configure, build, and test preset. `fail-fast: false` preserves diagnostics for both runtimes when one fails.

The existing Linux foundation build remains separate: it provides fast dependency-free feedback and does not duplicate the slower vcpkg jobs. Dependency services continue to validate real MySQL, Redis, and MongoDB data operations independently of compilation.
