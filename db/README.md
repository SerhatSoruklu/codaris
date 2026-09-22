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
