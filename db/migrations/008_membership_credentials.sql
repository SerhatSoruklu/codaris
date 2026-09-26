BEGIN;

CREATE TABLE app.membership_credentials (
    user_id BIGINT PRIMARY KEY REFERENCES app.accounts(user_id) ON DELETE CASCADE,
    membership_number VARCHAR(32) NOT NULL UNIQUE,
    verification_id UUID NOT NULL UNIQUE DEFAULT gen_random_uuid(),
    issued_at TIMESTAMPTZ,
    version SMALLINT NOT NULL DEFAULT 1 CHECK (version > 0),
    status TEXT NOT NULL DEFAULT 'pending' CHECK (status IN ('pending','active','suspended','revoked')),
    public_enabled BOOLEAN NOT NULL DEFAULT FALSE,
    consented_at TIMESTAMPTZ,
    created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CHECK ((public_enabled = FALSE) OR consented_at IS NOT NULL),
    CHECK ((status = 'pending' AND issued_at IS NULL) OR (status <> 'pending' AND issued_at IS NOT NULL))
);

-- Backfill verified and unverified accounts without exposing anyone by default.
-- Retry rare membership-number collisions; verification UUIDs come from PostgreSQL CSPRNG.
DO $$
DECLARE account_row RECORD;
DECLARE candidate TEXT;
DECLARE public_uuid UUID;
BEGIN
    FOR account_row IN
        SELECT a.user_id, a.email_verified
          FROM app.accounts a
         WHERE NOT EXISTS (SELECT 1 FROM app.membership_credentials c WHERE c.user_id=a.user_id)
         ORDER BY a.user_id
    LOOP
        LOOP
            public_uuid := gen_random_uuid();
            candidate := 'CDR-' || upper(substr(replace(public_uuid::text, '-', ''), 1, 16));
            BEGIN
                INSERT INTO app.membership_credentials(user_id,membership_number,verification_id,issued_at,status)
                VALUES (account_row.user_id,candidate,public_uuid,
                        CASE WHEN account_row.email_verified THEN CURRENT_TIMESTAMP ELSE NULL END,
                        CASE WHEN account_row.email_verified THEN 'active' ELSE 'pending' END);
                EXIT;
            EXCEPTION WHEN unique_violation THEN
                -- A conflicting public number is retried. UUID collisions are astronomically
                -- unlikely and are safe to retry through the same path.
                IF EXISTS (SELECT 1 FROM app.membership_credentials WHERE membership_number=candidate OR verification_id=public_uuid)
                   THEN CONTINUE;
                ELSE RAISE;
                END IF;
            END;
        END LOOP;
    END LOOP;
END $$;

INSERT INTO app.schema_migrations(version) VALUES ('008_membership_credentials');
COMMIT;
