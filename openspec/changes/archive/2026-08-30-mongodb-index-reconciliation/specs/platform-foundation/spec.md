# Change: Reconcile MongoDB AI interaction indexes at startup

## ADDED Requirements

### Requirement: MongoDB AI interaction indexes are reconciled during upgrade

The system SHALL reconcile required `ai_interactions` indexes before enabling the MongoDB AI interaction repository, so an existing database volume does not depend on first-run initialization scripts.

#### Scenario: Existing collection lacks the identity index
- **GIVEN** `storage.ai_interaction_store=mongodb` selects an existing `ai_interactions` collection
- **AND** the collection does not contain the required unique `interactionCode` index
- **WHEN** the backend creates its MongoDB AI interaction repository
- **THEN** it SHALL create the unique identity index and the related-object query index before serving interaction writes

#### Scenario: Historical duplicate identity blocks upgrade
- **GIVEN** existing `ai_interactions` documents contain duplicate `interactionCode` values
- **WHEN** the backend attempts to create the unique identity index
- **THEN** backend startup SHALL fail
- **AND** the error SHALL instruct the operator to deduplicate the conflicting interaction records before retrying
