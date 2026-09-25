# Database

PostgreSQL is the authoritative CODARIS datastore.

## Migration rule

- Applied migration files are immutable.
- Add a new numbered migration for every schema change.
- Run migrations as `codaris_app` in local development unless a migration explicitly requires an administrator operation.
- Never embed database passwords in SQL files or source code.

Current baseline:

- `app.users`
- `app.external_profiles`
- `app.schema_migrations`

External profile ownership verification is deliberately separate from storing a claimed profile URL.

Account migrations 002–004 add accounts, sessions, expiring action tokens, the encrypted mail outbox, request limits, learning progress and fixed-size avatar pixels. Run `scripts/db-migrate.py` with PG* environment values (or the Bash/PowerShell wrapper). Applied migrations are unchanged. Runtime queries use parameterized libpq. See [account design](../docs/ACCOUNT-SERVICE.md).
