# AI Diagnosis Assistance Storage Delta

## ADDED Requirements

### Requirement: AI interaction storage guidance uses the independent setting

AI documentation SHALL identify `ai_interaction_store` as the setting that selects AI interaction persistence, independently of `repository_store`.

#### Scenario: Operator configures AI interaction persistence
- **WHEN** an operator selects MySQL or MongoDB for AI interaction records
- **THEN** the documentation SHALL direct the operator to `ai_interaction_store`
- **AND** SHALL state that `repository_store` controls transactional business data separately
