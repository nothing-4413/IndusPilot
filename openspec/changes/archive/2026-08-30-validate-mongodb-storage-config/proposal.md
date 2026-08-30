# Change: Validate MongoDB storage configuration

## Why

The readiness layer can derive a MongoDB endpoint from `mongodb.host` and `mongodb.port` when the URI is empty, but the HTTP composition root previously passed the empty URI directly to the MongoDB repository. MongoDB storage also lacked static validation for its database and endpoint fields, allowing malformed configuration to fail before readiness could report a dependency problem.

## What Changes

- Validate the MongoDB database name whenever MongoDB AI interaction storage is selected.
- Validate host and port when the MongoDB URI is empty.
- Use the same host/port URI fallback in the HTTP composition root that readiness uses.
- Add configuration regression coverage and documentation.

## Non-Goals

- No URI parser or credential validation is added.
- No change to MongoDB authentication or readiness semantics.
