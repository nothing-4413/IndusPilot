# Change: Align configurable MySQL database names

## Why

Compose and backend configuration expose a configurable MySQL database name, while migration and integration SQL force `induspilot`. A deployment using another database name can start successfully but initialize and verify a different schema from the one used by the backend.

## What Changes

- Make migration and integration SQL rely on the selected MySQL database rather than a hard-coded `USE` statement.
- Make the migration runner create/select the configured database safely and use it for every script.
- Make dependency smoke pass the Compose database name through all MySQL commands.
- Add preflight checks and documentation for the shared database-name contract.

## Non-Goals

- No migration rollback or schema renaming for an already deployed database.
- No change to MySQL table or column semantics.
- No support for arbitrary SQL identifier syntax in environment variables.
