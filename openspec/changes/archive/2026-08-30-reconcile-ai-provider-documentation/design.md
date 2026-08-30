# Design

Use the implemented runtime contract as the source of truth: `ai.enabled=true` and `ai.provider=http` invoke the configured endpoint in Drogon builds, share a total timeout budget across bounded retries, enforce response limits, and fall back to local rules on failure. Correct malformed field names and leave only genuinely unimplemented model/provider extensions as future work.
