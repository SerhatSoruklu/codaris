# Database

PostgreSQL is the authoritative CODARIS datastore.

## Migration rule

- Applied migration files are immutable.
- Add a new numbered migration for every schema change.
- Run migrations as `codaris_app` in local development unless a migration explicitly requires an administrator operation.
- Never embed database passwords in SQL files or source code.
- In production, `codaris_migrator` owns the database/schema and applies reviewed migrations. The `codaris_app` runtime role is separate, non-superuser, and receives only the table/sequence privileges required by the API and mail worker from `scripts/db-live-migrate.sh`.
- When a migration adds a table or sequence used by the application, update the explicit runtime grants in `scripts/db-live-migrate.sh` in the same change. Do not make the runtime role an owner or grant it schema creation rights.

Current baseline:

- `app.users`
- `app.external_profiles`
- `app.schema_migrations`

External profile ownership verification is deliberately separate from storing a claimed profile URL.

Account migrations 002–004 add accounts, sessions, expiring action tokens, the encrypted mail outbox, request limits, learning progress and fixed-size avatar pixels. Run `scripts/db-migrate.py` with PG* environment values (or the Bash/PowerShell wrapper). Applied migrations are unchanged. Runtime queries use parameterized libpq. See [account design](../docs/ACCOUNT-SERVICE.md).
