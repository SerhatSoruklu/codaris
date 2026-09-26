BEGIN;
ALTER TABLE app.sessions
    ALTER COLUMN expires_at SET DEFAULT CURRENT_TIMESTAMP + INTERVAL '28 days';
UPDATE app.sessions
   SET expires_at = CURRENT_TIMESTAMP + INTERVAL '28 days'
 WHERE expires_at > CURRENT_TIMESTAMP;
INSERT INTO app.schema_migrations(version) VALUES ('010_session_lifetime');
COMMIT;
