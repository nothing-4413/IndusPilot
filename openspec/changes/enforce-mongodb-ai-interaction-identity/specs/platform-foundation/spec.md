# Change: Enforce MongoDB AI interaction identity

## ADDED Requirements

### Requirement: MongoDB AI interaction idempotency key is unique

The system SHALL enforce `interactionCode` as a unique identity key in the MongoDB `ai_interactions` collection.

#### Scenario: Concurrent or repeated interaction write
- **GIVEN** an AI interaction document already exists with an `interactionCode`
- **WHEN** a writer retries the same interaction or another writer attempts a conflicting insert
- **THEN** the repository upsert SHALL update the existing document
- **AND** MongoDB SHALL reject a distinct duplicate document with the same `interactionCode`
