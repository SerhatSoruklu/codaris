BEGIN;
CREATE TABLE app.accounts (
 user_id BIGINT PRIMARY KEY REFERENCES app.users(id) ON DELETE CASCADE,
 email VARCHAR(254) NOT NULL CHECK(email = lower(email) AND email ~ '^[^[:space:]@]+@[^[:space:]@]+\.[^[:space:]@]+$'),
 password_hash TEXT NOT NULL,
 email_verified BOOLEAN NOT NULL DEFAULT FALSE,
 country VARCHAR(120) NOT NULL CHECK(length(country) BETWEEN 2 AND 120),
 role VARCHAR(64) NOT NULL CHECK(role IN ('Developer / Engineer','Researcher','Community organiser','Student','Other')),
 motivation TEXT NOT NULL CHECK(length(motivation) BETWEEN 20 AND 2000),
 linkedin TEXT NOT NULL DEFAULT '', github TEXT NOT NULL DEFAULT '', website TEXT NOT NULL DEFAULT '',
 CHECK(linkedin = '' OR linkedin LIKE 'https://%'),
 CHECK(github = '' OR github LIKE 'https://%'),
 CHECK(website = '' OR website LIKE 'https://%')
);
CREATE UNIQUE INDEX accounts_email_unique ON app.accounts(lower(email));
CREATE FUNCTION app.keep_application_reason() RETURNS trigger LANGUAGE plpgsql AS $$
BEGIN
 IF NEW.motivation IS DISTINCT FROM OLD.motivation THEN
  RAISE EXCEPTION 'The original application reason cannot be changed';
 END IF;
 RETURN NEW;
END $$;
CREATE TRIGGER immutable_application_reason BEFORE UPDATE ON app.accounts
 FOR EACH ROW EXECUTE FUNCTION app.keep_application_reason();
CREATE TABLE app.sessions (
 token_hash CHAR(64) PRIMARY KEY,
 user_id BIGINT NOT NULL REFERENCES app.accounts(user_id) ON DELETE CASCADE,
 expires_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP + INTERVAL '12 hours'
);
CREATE INDEX sessions_user_idx ON app.sessions(user_id);
CREATE TABLE app.action_tokens (
 token_hash CHAR(64) PRIMARY KEY,
 user_id BIGINT NOT NULL REFERENCES app.accounts(user_id) ON DELETE CASCADE,
 kind TEXT NOT NULL CHECK(kind IN ('verify','reset')),
 email VARCHAR(254) NOT NULL,
 expires_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP + INTERVAL '24 hours'
);
CREATE INDEX action_tokens_user_idx ON app.action_tokens(user_id);
CREATE TABLE app.mail_outbox (
 id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
 user_id BIGINT NOT NULL REFERENCES app.accounts(user_id) ON DELETE CASCADE,
 recipient VARCHAR(254) NOT NULL,
 kind TEXT NOT NULL CHECK(kind IN ('verify','reset','email-changed','password-changed')),
 encrypted_token TEXT NOT NULL DEFAULT '',
 attempts INTEGER NOT NULL DEFAULT 0,
 available_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
 created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
 sent_at TIMESTAMPTZ
);
CREATE INDEX mail_pending_idx ON app.mail_outbox(available_at) WHERE sent_at IS NULL AND attempts < 8;
CREATE TABLE app.rate_limits (
 bucket TEXT PRIMARY KEY,
 window_start TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
 attempts INTEGER NOT NULL DEFAULT 1
);
INSERT INTO app.schema_migrations(version) VALUES ('002_accounts');
COMMIT;
