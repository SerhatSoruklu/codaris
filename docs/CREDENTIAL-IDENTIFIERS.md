# Membership credential identifiers

Each credential has two separate identifiers:

- `membership_number` is a public, human-readable display number (`CDR-` plus 16 uppercase characters).
- `verification_id` is a UUID used in verification links and credential barcodes. It is a secret-like bearer identifier and must not be derived from the display number.

New registrations already generate these values independently in `credential_random()`.

Migration `009_cred_numbers` repairs the older backfilled rows from migration 008. It selects only rows whose display number exactly matches the first 16 hexadecimal characters of that row's `verification_id`, assigns a new independently generated public number, and leaves `verification_id` unchanged. Unique-number collisions are retried. It also updates `updated_at` for affected rows. Existing verification links and QR/barcode payloads therefore remain valid; the displayed membership number changes on repaired rows.

Apply this migration to a disposable local database first, then a non-production environment, and review the affected-row count before any live rollout. Do not run it directly against production as part of this credential rendering change. Migration 008 remains immutable. The local database was not available for a read-only row count in this worktree: the local TCP connection required an unset database password, and the local Unix socket had no `serhat` PostgreSQL role.
