BEGIN;
CREATE TABLE app.learning_progress (
 user_id BIGINT NOT NULL REFERENCES app.accounts(user_id) ON DELETE CASCADE,
 topic SMALLINT NOT NULL CHECK(topic BETWEEN 0 AND 21),
 read_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
 PRIMARY KEY(user_id,topic)
);
INSERT INTO app.schema_migrations(version) VALUES ('003_learning_progress');
COMMIT;
