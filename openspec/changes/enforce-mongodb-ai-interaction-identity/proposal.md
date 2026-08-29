# Change: Enforce MongoDB AI interaction identity

## Why

MongoDB AI interaction writes use `interactionCode` as their idempotency key, but the collection does not enforce that key as unique. Concurrent retries could create duplicate interaction documents.

## What Changes

- Create a unique `interactionCode` index for `ai_interactions` during MongoDB initialization.
- Extend the real MongoDB smoke script to assert the unique index and duplicate-write rejection.

## Impact

- Affected specs: `platform-foundation`
- Affected files: MongoDB initialization and real CRUD smoke validation
