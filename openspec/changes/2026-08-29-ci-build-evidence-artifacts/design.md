# Design: CI build evidence artifacts

Each job keeps its existing checkout and build directory. An `actions/upload-artifact@v4` step runs with `if: always()` after the test steps, names artifacts by job and matrix preset, and uploads only the backend executable pattern plus `Testing/Temporary/LastTest.log`. Missing files are warnings because a configure or build failure may prevent the output from being created. Retention is set to 14 days.

The upload paths are explicit for `build/ci-backend`, `build/${{ matrix.preset }}`, and `build/dev-http`. The workflow does not include `deployment/.env`, repository source files, or Docker data paths. This provides failure evidence without turning CI artifacts into a release channel.
