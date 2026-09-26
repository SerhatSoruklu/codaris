BEGIN;

-- Keep contact text encrypted at rest until both fixed-destination emails are
-- accepted or the 30-day retry/retention limit is reached.
CREATE TABLE app.contact_outbox (
  id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  encrypted_payload TEXT NOT NULL,
  created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
  available_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
  admin_sent_at TIMESTAMPTZ,
  receipt_sent_at TIMESTAMPTZ,
  admin_attempts INTEGER NOT NULL DEFAULT 0 CHECK (admin_attempts BETWEEN 0 AND 8),
  receipt_attempts INTEGER NOT NULL DEFAULT 0 CHECK (receipt_attempts BETWEEN 0 AND 8)
);

CREATE INDEX contact_outbox_pending_idx
  ON app.contact_outbox(available_at, id)
  WHERE admin_sent_at IS NULL OR receipt_sent_at IS NULL;

INSERT INTO app.schema_migrations(version) VALUES ('006_contact_outbox');

COMMIT;
