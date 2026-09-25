BEGIN;
ALTER TABLE app.accounts ADD COLUMN avatar_rgba BYTEA;
ALTER TABLE app.accounts ADD CONSTRAINT avatar_fixed_pixels CHECK(avatar_rgba IS NULL OR octet_length(avatar_rgba)=40000);
INSERT INTO app.schema_migrations(version) VALUES ('004_avatar');
COMMIT;
