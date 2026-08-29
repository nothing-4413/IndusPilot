# Design: Shared MySQL database selection

All migration and integration SQL files omit `USE induspilot;`. The Docker MySQL entrypoint selects `MYSQL_DATABASE` for initialization, and manual execution supplies the database as the mysql client argument.

The migration runner validates `MYSQL_DATABASE` against a conservative identifier pattern (`[A-Za-z0-9_$]+`), creates it when needed using a quoted identifier, and passes it to every migration invocation. This keeps the identifier out of shell interpolation and prevents injection through deployment configuration. The foundation migration no longer needs to create or select a fixed database.

Dependency smoke reads the database name from the Compose container environment for migration verification and CRUD smoke. The backend already reads the same deployment value through its MySQL URI/configuration, so a custom name is now applied consistently end to end.
