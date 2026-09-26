BEGIN;

-- Migration 008 derived backfilled public numbers from the verifier UUID.
-- Replace only those exact derived values. The verifier UUID is never updated.
DO $$
DECLARE
    credential_row RECORD;
    candidate TEXT;
BEGIN
    FOR credential_row IN
        SELECT user_id, verification_id
          FROM app.membership_credentials
         WHERE membership_number = 'CDR-' ||
               upper(substr(replace(verification_id::text, '-', ''), 1, 16))
         ORDER BY user_id
    LOOP
        LOOP
            -- Generate a fresh random UUID solely as input to the display-number
            -- encoding; it is independent from the stored verification_id.
            candidate := 'CDR-' || upper(substr(replace(gen_random_uuid()::text, '-', ''), 1, 16));
            BEGIN
                UPDATE app.membership_credentials
                   SET membership_number = candidate,
                       updated_at = CURRENT_TIMESTAMP
                 WHERE user_id = credential_row.user_id;
                EXIT;
            EXCEPTION WHEN unique_violation THEN
                -- Retry the vanishingly unlikely collision against any public number.
                CONTINUE;
            END;
        END LOOP;
    END LOOP;
END $$;

INSERT INTO app.schema_migrations(version) VALUES ('009_cred_numbers');
COMMIT;
