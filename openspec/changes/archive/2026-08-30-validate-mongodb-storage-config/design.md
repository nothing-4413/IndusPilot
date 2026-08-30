# Design: Validate MongoDB storage configuration

`validateConfig` treats `storage.ai_interaction_store=mongodb` as requiring a non-empty `mongodb.database`. If `mongodb.uri` is empty, it additionally requires a non-empty host and a port in `1..65535`; a non-empty URI remains the authoritative connection target. The HTTP composition root constructs `mongodb://<host>:<port>` when the URI is empty, matching `DataConnectors::probe` and preserving URI-based authentication when configured.
