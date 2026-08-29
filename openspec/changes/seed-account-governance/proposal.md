# Change: Govern seed account credentials

## Why

The MySQL initialization scripts and in-memory repository contain documented demonstration credentials. Production deployment guidance asks operators to replace them manually, but the application has no durable record of whether rotation happened and no guard that prevents the published credentials from being used.

## What Changes

- Track whether a user credential must be rotated before ordinary sign-in can be used.
- Mark all database seed accounts as rotation-required, while retaining an explicit development compatibility mode for local demonstrations.
- Reject ordinary login for unrotated seed accounts in production-oriented configuration without exposing credential data.
- Add a deployment credential-rotation utility and preflight checks for the schema, seed state, and production-safe configuration.
- Cover configuration, identity behavior, database migration, and deployment tooling with tests and documentation.

## Non-Goals

- No external IdP, MFA, self-service recovery, or user provisioning UI.
- No automatic password generation or transmission of passwords through logs, HTTP responses, or source control.
