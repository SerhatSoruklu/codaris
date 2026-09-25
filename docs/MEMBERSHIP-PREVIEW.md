# Historical membership preview

This document describes the previous browser-only prototype. It is superseded by [ACCOUNT-SERVICE.md](ACCOUNT-SERVICE.md) and [RELEASE.md](RELEASE.md). The current account service persists data and sends queued transactional mail. Do not use the old operational instructions below.

# Membership development preview

## Implemented boundary

This is an interaction skeleton, not an authentication system. The native C backend and PostgreSQL schema have not changed. There is no upload endpoint, account record, password storage, SMTP connection, payment or renewal workflow. OVH/COUPYN integration is intentionally deferred. A CSS blur is never an access-control mechanism.

The shared Python builder uses `CODARIS_PRODUCTION=1` for production. It emits the existing inert, disabled and blurred application on `/join/`, omits development-only controls, and replaces `/login/` and `/dashboard/` with coming-soon pages without a runtime. Development builds show the editable application and interactive member preview. The deployment packager rejects development HTML; rebuild with `CODARIS_PRODUCTION=1` before packaging. Both membership routes remain noindex and are excluded from the sitemap. The Login link is in the header; Join remains a call to action rather than a navigation/footer item.

Form controls start disabled and are enabled after Wasm initialization only in a development document. Runtime failure leaves the preview unavailable. `form-action 'none'` remains in the CSP. Native browser validity handles required values and field lengths; C handles mismatch results, page state, topic progress and photo metadata policy. JavaScript transports browser events, decodes local image files and draws previews in a canvas. No network calls transmit form values, no localStorage/sessionStorage is used, and no credentials are persisted by application code.

Profile pictures: JPEG, PNG, WebP, at most 5 MiB, decoded dimensions at most 8192 per side. Selection replaces the prior picture, failures clear it, stale asynchronous decodes cannot overwrite a newer selection, and ImageBitmap resources are closed. The picker performs a centre-cropped local preview, not an upload. Browser-only validation must never become the production upload security boundary.

The dashboard shows a random 24-character Crockford-style ID, using browser Web Crypto randomness and C formatting. It is a demonstration identifier, not an issued membership ID, authentication secret or guarantee of uniqueness. Every new document gets a fresh sample. The future server must issue permanent IDs with a PostgreSQL unique constraint and retry on collision. The application preview does not create a logged-in state; the dashboard can be opened directly as a public development demo.

The UI draft follows the requested membership-ID login direction, accompanied by a password, with email recovery. The final identity/authentication design remains a prerequisite for backend implementation. The preview never authenticates entered passwords; it reports that login is disconnected. Security settings clear password fields after a successful preview. Password comparisons here are form-confirmation checks, not authentication primitives.

## Routes and interactions

- `/join/`: picture picker, existing application fields, sample password/confirmation, validation, preview result and dashboard link.
- `/login/`: login layout, generic recovery result and a distinct Open dashboard preview link.
- `/dashboard/`: overview and membership card; profile, location and role; password/email-change preview; learning cards with keyboard-accessible tabs and per-document explored markers.
- Learning cards link to primary resources: [Python tutorial](https://docs.python.org/3/tutorial/), [Angular tutorials](https://angular.dev/tutorials), [MDN web learning](https://developer.mozilla.org/en-US/docs/Learn_web_development), and [PostgreSQL tutorial](https://www.postgresql.org/docs/current/tutorial.html). Software development and system design include local starter prompts. These are starting topics, not a complete course catalog.
- `templates/email/`: paired HTML/text drafts for application receipt, email verification, approval, password reset/change, email verification/change and ID reminder. Templates are not in public build output. Their README describes rendering and delivery responsibilities.

## Future backend design — not implemented

Before enabling real registration, decide approval/account states, identity normalization and recovery, data retention and deletion, provider contract, and mature C dependencies. Define a threat model and schema before adding sequential migrations. Do not reuse preview IDs or browser validation as trusted input.

Proposed account lifecycle: application received → email verified → under review → approved/rejected. Approval and email verification must be enforced server-side before privileged member actions. Account identity, applicant status and issued membership should be separate durable concepts. Email changes require reauthentication, verification of the new address, and notification to the previous address. Password recovery should have generic responses, expiring single-use tokens and abuse controls.

Production authentication needs password hashing through a vetted library, rate limits, MFA/passkey planning, server-enforced authorization, secure session cookies and CSRF protection. Do not use a membership ID as the sole authenticator. Passwords must never be emailed or logged. The UI's sample 15–128-character rule is not a complete password policy. See [OWASP authentication guidance](https://cheatsheetseries.owasp.org/cheatsheets/Authentication_Cheat_Sheet.html) and [password storage guidance](https://cheatsheetseries.owasp.org/cheatsheets/Password_Storage_Cheat_Sheet.html).

Real avatars require server-side file identification, bounded decoding, re-encoding, metadata removal, ownership checks, storage isolation, quotas and safe delivery headers. Do not trust file extensions or browser MIME declarations. See [OWASP file upload guidance](https://cheatsheetseries.owasp.org/cheatsheets/File_Upload_Cheat_Sheet.html). All persistence must use parameterized libpq calls from native C; database/provider credentials stay server-side.

Future mail integration needs a durable event queue, idempotent retries, token lifecycle, verified sender, delivery failure handling and provider-specific configuration. The browser must not talk to SMTP. No new dependency is selected in this preview.
