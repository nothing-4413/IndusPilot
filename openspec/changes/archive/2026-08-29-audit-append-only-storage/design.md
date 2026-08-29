# Design: Append-only operation audit storage

`InMemoryOperationAuditRepository::save` checks the event code before appending. If it already exists, it returns the stored event unchanged. This models the production invariant without throwing from the repository interface.

`MySqlOperationAuditRepository::save` uses `INSERT IGNORE`, relying on the existing unique `event_code` constraint to make concurrent duplicate writes non-mutating. It then selects the event by code and returns the persisted row, so callers never receive a duplicate input as if it had been stored. No audit fields are updated after insertion.
