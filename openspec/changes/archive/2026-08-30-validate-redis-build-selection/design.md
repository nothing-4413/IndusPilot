# Design: Redis build capability validation

Redis is an optional compile-time capability. `validateConfig` will reject both configuration selectors that need it when `INDUSPILOT_WITH_REDIS` is absent. The default build therefore fails before running rather than changing a declared Redis session store into an in-memory store.

`runDrogonServer` will validate the configuration before `buildHttpServerContext`, so invalid static configuration does not allocate clients or create services. `createSessionStore` retains a defensive runtime error for callers that bypass the normal server entry point.

The default foundation profile asserts that Redis selectors are rejected, while the Redis-enabled profile asserts the equivalent selector is valid. A focused config-capability CTest covers that contract in both profiles without coupling it to the broader integration scenarios. Feature macros are applied directly to the foundation test target for each enabled core capability, so its Redis and webhook assertions select the same contracts that the core implementation compiled. The malformed-file fixture is created in the test working directory rather than resolving a platform temporary directory, avoiding environment-specific filesystem stalls in unattended runs.
