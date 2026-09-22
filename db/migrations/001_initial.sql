BEGIN;

CREATE SCHEMA IF NOT EXISTS app;

CREATE TABLE app.schema_migrations (
  version VARCHAR(32) PRIMARY KEY,
  applied_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE app.users (
  id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  username VARCHAR(50) NOT NULL,
  display_name VARCHAR(120) NOT NULL,
  created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE UNIQUE INDEX users_username_lower_unique
  ON app.users (LOWER(username));

CREATE TABLE app.external_profiles (
  id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  user_id BIGINT NOT NULL
    REFERENCES app.users(id)
    ON DELETE CASCADE,
  provider VARCHAR(32) NOT NULL,
  profile_url TEXT NOT NULL,
  handle VARCHAR(255),
  verified BOOLEAN NOT NULL DEFAULT FALSE,
  verified_at TIMESTAMPTZ,
  visibility VARCHAR(16) NOT NULL DEFAULT 'public',
  created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,

  CONSTRAINT external_profiles_provider_not_empty
    CHECK (LENGTH(TRIM(provider)) > 0),
  CONSTRAINT external_profiles_visibility_valid
    CHECK (visibility IN ('public', 'private')),
  CONSTRAINT external_profiles_verified_timestamp_valid
    CHECK ((verified = FALSE AND verified_at IS NULL) OR (verified = TRUE AND verified_at IS NOT NULL)),
  CONSTRAINT external_profiles_user_provider_url_unique
    UNIQUE (user_id, provider, profile_url)
);

CREATE INDEX external_profiles_user_id_idx
  ON app.external_profiles(user_id);

CREATE INDEX external_profiles_provider_idx
  ON app.external_profiles(provider);

INSERT INTO app.schema_migrations(version) VALUES ('001_initial');

COMMIT;
