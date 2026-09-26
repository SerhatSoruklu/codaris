BEGIN;
ALTER TABLE app.accounts DROP CONSTRAINT IF EXISTS accounts_role_valid;
ALTER TABLE app.accounts ADD CONSTRAINT accounts_role_valid CHECK (role IN (
  'Developer / Engineer',
  'AI / ML Engineer',
  'Security Engineer / Researcher',
  'Researcher',
  'Systems / Infrastructure Engineer',
  'Technical Founder / Entrepreneur',
  'Open-source Maintainer / Contributor',
  'Product / UX Designer',
  'Educator / Technical Writer',
  'Community organiser',
  'Student',
  'Other'
));
INSERT INTO app.schema_migrations(version) VALUES ('007_role_choices');
COMMIT;
