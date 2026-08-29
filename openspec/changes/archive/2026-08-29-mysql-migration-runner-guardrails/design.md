# Design: MySQL migration runner guardrails

`database/mysql/migrate.sh` is the single execution entrypoint. It uses the standard `mysql` client and environment variables (`MYSQL_HOST`, `MYSQL_PORT`, `MYSQL_USER`, `MYSQL_PWD`, and `MYSQL_DATABASE`) so CI, containers, and operators can invoke the same behavior. The database defaults to `induspilot`, matching the existing scripts.

The runner discovers files matching `NNN_name.sql`, sorts them lexically, applies the first foundation script to create the database and version table, then reads `schema_migrations`. Before each pending script it rejects any recorded version that is not present in the local directory and rejects a later recorded version when an earlier version is missing. After execution it verifies that the script registered its expected version. Already registered versions are skipped.

The runner does not pass a password on the command line. `MYSQL_PWD` is inherited by the client process and is unset by the caller after use. Existing migrations remain responsible for their own idempotent DDL and version registration; the runner supplies ordering and state validation around them.
