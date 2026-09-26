# Account service design

C17 native API, PostgreSQL authoritative storage through parameterized libpq, C/Wasm client. Same-origin `/api/` behind Nginx TLS; native HTTP binds loopback only. No database or SMTP values enter frontend output.

Registration creates an unverified account with an Argon2id password hash and a permanent random membership ID. A single-use 24-hour email token activates membership automatically. Applicants can log in and edit their own settings before verification; member-only operations must check current email verification in PostgreSQL. Changing email requires the current password, immediately replaces email, clears verification, revokes other sessions and old verification/reset tokens, queues a new verification link and a notice to the old address. Email uniqueness is case-insensitive; ASCII email addresses only in this release. Names accept UTF-8. Motivation is immutable after application, enforced by a database trigger. External URLs are HTTPS claims, never ownership verification.

Sessions are random 256-bit opaque cookies, HttpOnly, SameSite=Strict and Secure in production, expiring after 28 days. Only token hashes are persisted for sessions. `GET /api/session` returns an empty 204 for a valid session or 401 otherwise; the shared header uses this minimal response and does not fetch the member profile. Mutation requests require an exact configured Origin and application/json, protecting against cross-origin form submission. Session IDs rotate on login; password reset/change revokes all sessions. Token consumption and account mutation are one database transaction with row locks. Verification/reset links carry tokens in a URL fragment; confirmation requires POST so mail scanners cannot consume them with GET. Recovery gives a generic response. Rate limits persist in PostgreSQL; reverse-proxy limits supplement them.

A durable mail outbox is committed with account changes. It stores encrypted action tokens (libsodium secretbox), never passwords; the encryption key is backend-only. Delivery workers decrypt only while sending, retry with bounded attempts, erase payload on success, and use a stable Message-ID. SMTP delivery is at-least-once: a crash after acceptance may repeat a message. Tokens remain single-use. SMTP requires TLS with certificate validation in production. Local development uses a loopback SMTP capture service. No SMTP credentials are copied from other applications.

## Native dependencies

- GNU libmicrohttpd, LGPL-2.1-or-later: HTTP parsing and bounded request handling. C17 has no HTTP server. https://www.gnu.org/software/libmicrohttpd/
- libsodium, ISC: Argon2id, random tokens, digests and authenticated encryption. No custom cryptography. https://doc.libsodium.org/
- libcurl, curl license: SMTP/TLS, MIME construction and URL parsing; C17/libpq cannot provide these. https://curl.se/libcurl/
- json-c, MIT: JSON parsing and serialization with explicit lengths; avoids a custom JSON parser. https://github.com/json-c/json-c
- Existing PostgreSQL/libpq, PostgreSQL license: durable relational data.

Use OS security-maintained packages or a pinned Windows package manager; keep all libraries patched. Linux packages: libpq-dev libmicrohttpd-dev libsodium-dev libcurl4-openssl-dev libjson-c-dev. Windows: vcpkg libpq libmicrohttpd libsodium curl json-c, with the vcpkg CMake toolchain. No new browser dependencies.

## Release prerequisites

Production needs an isolated PostgreSQL database/role, TLS reverse proxy, private environment configuration, mail encryption key, verified sender and SMTP credentials. Existing inbound mail forwarding is not outbound SMTP. Google Workspace SMTP uses the expressly selected admin@coupyn.com identity, matching the Serhat application. Backend-only credentials are isolated from frontend output. Privacy/terms, abuse handling, backups/restore, deliverability and real production verification must be completed before public registration is advertised. Unimplemented chat/upload/staff actions must not masquerade as working services.

Avatars are exactly 100×100 RGBA pixels (40,000 decoded bytes), validated server-side and constrained in PostgreSQL. Browser decoding strips file metadata by producing a bounded pixel buffer. Original image files never reach storage. Public membership metrics query verified accounts only; learning progress is per-user durable state.

### Login and recovery validation

Password creation/reset uses 15–128 Unicode code points (maximum 512 UTF-8 bytes). Browser password fields allow up to 512 UTF-16 units so they cannot truncate a backend-valid password; the C backend enforces the authoritative limits. Login compares passwords exactly, including case and whitespace. New membership IDs contain 24 characters from `0123456789ABCDEFGHJKMNPQRSTVWXYZ`, matched case-insensitively; login does not impose this format on existing stored usernames.

Recovery uses an email keyboard and autocomplete, but delegates email syntax to the backend so browser-specific email rules do not disagree with it. Invalid addresses receive an email-specific inline message. Valid-address responses stay generic even if the account-specific mail queue fails; the transaction rolls back and the service logs a non-identifying diagnostic. This does not claim constant-time recovery responses.
