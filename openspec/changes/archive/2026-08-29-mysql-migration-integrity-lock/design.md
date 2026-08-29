# Design: Checksummed migration session

Migration `014_migration_integrity.sql` adds a nullable SHA-256 checksum column. The runner backfills entries established before this upgrade after `014` succeeds, then verifies all recorded checksums before applying future migrations. A missing checksum is only backfilled for a matching known local script; a non-empty differing checksum is a hard failure.

The runner starts one long-lived mysql batch client connected to the selected schema. It obtains `GET_LOCK` after database creation and keeps that session open while schema state is inspected, migration SQL is streamed, and checksums are written. A deferred cleanup releases the lock and closes the client. This makes the advisory lock meaningful across concurrent runners.
