# Change: Protect MySQL migration integrity and concurrency

## Why

Ordered migration execution does not detect an edited script after it was deployed, and independent deployment processes can still race while applying migrations.

## What Changes

- Add a checksum column to migration history.
- Record and verify SHA-256 checksums for every local migration.
- Use a MySQL advisory lock held in the same client session that applies migrations.
- Extend fake-client, preflight, and operator documentation coverage.

## Non-Goals

- No automatic rollback.
- No rewriting of historic migration SQL.
- No distributed lock outside MySQL.
