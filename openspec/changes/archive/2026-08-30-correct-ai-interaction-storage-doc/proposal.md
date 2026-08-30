# Change: Correct AI interaction storage guidance

## Why

The AI architecture document attributes AI interaction persistence to `repository_store`, although the runtime uses the independent `ai_interaction_store` setting. This can send operators to the wrong storage configuration.

## What Changes

- Correct the configuration key and document the supported memory, MySQL, and MongoDB AI interaction stores.
- Add a specification scenario protecting the independent storage boundary.

## Impact

- Affected specs: `ai-diagnosis-assistance`
- Affected areas: AI architecture documentation
