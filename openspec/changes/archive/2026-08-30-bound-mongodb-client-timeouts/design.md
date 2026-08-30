# Design: Bound MongoDB repository client timeouts

The composition root passes `readiness.probe_timeout_ms` to `MongoAiInteractionRepository`. The repository clamps the value to at least one millisecond and appends `serverSelectionTimeoutMS` and `connectTimeoutMS` to the client URI through the existing bounded URI helper. The same bounded URI is used for the repository client and the constructor's authenticated probe, so deferred index reconciliation observes the same upper bound when the dependency recovers or remains unavailable.
