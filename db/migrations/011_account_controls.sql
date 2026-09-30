BEGIN;

-- Track the current login session so the member menu can show its age.
ALTER TABLE app.sessions
    ADD COLUMN created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP;

-- Permanent account deletion removes the parent user row; existing foreign-key
-- cascades then remove the account, sessions, credentials, profile links and progress.
GRANT DELETE ON app.users TO codaris_app;

INSERT INTO app.schema_migrations(version) VALUES ('011_account_controls');
COMMIT;
