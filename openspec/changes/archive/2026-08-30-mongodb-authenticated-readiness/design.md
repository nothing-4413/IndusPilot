# Design

Expose a small `probeMongoDb` function from the MongoDB adapter translation unit so the dependency layer can use the same driver initialization boundary as the repository. The probe creates a driver client with `serverSelectionTimeoutMS` and `connectTimeoutMS` bounded by the configured readiness timeout, then runs an authenticated `{ping: 1}` command against the configured database.

The dependency probe remains parallel with other required dependencies and keeps one absolute readiness deadline. Driver failures are converted into the existing `ProbeResult` shape; no exception escapes the readiness request. Builds without `INDUSPILOT_WITH_MONGODB` retain the existing TCP probe path, although configuration validation prevents selecting MongoDB in those builds.
