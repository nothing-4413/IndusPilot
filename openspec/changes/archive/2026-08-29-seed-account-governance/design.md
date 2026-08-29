# Design: Seed account governance

## Credential state

The `users` table gains a `requires_password_rotation` boolean. It is set for the known initial accounts only. The state is returned through the repository credential boundary rather than inferred from a password hash, so operators can rotate to any valid PBKDF2 credential without application changes.

`UserCredential` carries this state. The in-memory repository marks its three demo identities as rotation-required as well, preserving an accurate model of the development fixtures.

## Login policy

`security.allow_seed_credentials` is `false` by default. When a credential requires rotation and this switch is disabled, `IdentityService::login` rejects the request with `SEED_CREDENTIAL_ROTATION_REQUIRED`; no session is created. The HTTP boundary maps this result to `403` and records an audit event without password or hash fields.

The example local configuration explicitly enables the switch because it uses the in-memory demonstration identities. Production configurations must leave it disabled. This preserves local demos while making an accidental production use of a known seed account fail closed.

## Rotation operation

`deployment/rotate_seed_credentials.ps1` receives a MySQL connection target and passwords through PowerShell secure-string parameters. It creates a fresh random salt and PBKDF2-SHA256 hash locally, updates the selected account atomically, increments `credential_version`, clears `requires_password_rotation`, and prints no credential value or hash.

The operation is intentionally database-admin controlled rather than exposed as an unauthenticated HTTP bootstrap endpoint: a documented seed password must never become a first-writer-wins administrator takeover path.

## Migration and compatibility

Migration `012_seed_account_governance.sql` adds the rotation column only when absent and marks only rows whose usernames and current hashes match the published seed identities. It remains idempotent and does not alter an already-rotated account.

Existing user-created and rotated accounts default to `false`. Session credential versions are incremented by the rotation utility, so any earlier session fails the existing credential-version validation.
