# Design

The MongoDB AI interaction repository is constructed only when `storage.ai_interaction_store=mongodb` is selected. Its constructor is therefore the narrowest point at which the service can reconcile schema requirements before accepting interaction writes.

The repository will issue idempotent `createIndexes` operations for the unique ascending `interactionCode` index and the `(relatedType, relatedId, createdAt desc)` query index. MongoDB accepts repeated requests when the existing key pattern and options match. A failure is rethrown as a startup exception that names the collection and instructs operators to deduplicate conflicting `interactionCode` values and recreate the required unique index.

The real runtime profile will deliberately remove the unique index after initialization and before the backend starts. A successful profile proves that a pre-existing MongoDB collection is repaired by the application rather than depending on Docker's first-volume initialization hook. The profile will verify that the recreated index is unique after the HTTP smoke completes.

The Docker initialization script remains useful for empty deployments and direct collection consumers; startup reconciliation closes the upgrade gap.
