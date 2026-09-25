> Historical design: staff tools are now unavailable in both build modes. The current beta uses automatic email-verified membership; see [RELEASE.md](RELEASE.md).

# Staff portal preview

The only staff roles are Administrator, Moderator and Support. A normal member has no staff role. A person's editable development discipline (developer, researcher, etc.) is unrelated to staff access.

## Implemented

- Main-site local previews: `/staff-login/` and `/staff-dashboard/`.
- Separate admin build: `build/admin/index.html`, `/login/`, `/dashboard/`, styles/runtime/assets, robots exclusion and a 404 page. The shared builder runs on Bash and PowerShell. The canonical origin is `https://admin.codaris.org`.
- Member Login links to Staff Login. The normal dashboard displays a role badge and staff-workspace link for a selected staff example. The fixture selector is explicitly a development preview, never a role assignment facility.
- Admin login has a membership-ID-or-email field and password, custom inline field errors and a generic unavailable result. No OS dialogs or native validity popovers are used. No two-factor step is implemented, as requested.
- All three roles can preview the same three fictional applications, search by name/application ID, filter status and read detail. No approvals, role changes, notes or live applications are persisted.
- Direct entry without a recognized staff example denies the preview. Hash fragments select fictional staff examples, not authentication sessions. C owns the fixture role, filter and selection logic; browser JS only transports events.
- Production renders unavailable pages for all member/staff preview routes with no interactive runtime. Main-site staff pages remain noindex and out of the sitemap; the separate admin build is noindex/nofollow and disallows crawling. The packager refuses development HTML for either site.

The current normal application form does not submit to this queue. The queue contains only examples. Browser role checks are presentation only and provide no security boundary. No real credential is accepted and no access token is issued. Changing a fragment never grants real access.

## Preview locally

Run the normal client build. Use `http://localhost:8080/staff-login/` on the main development server, or run `python3 -m http.server 8088 --bind 127.0.0.1 --directory build/admin` and visit `http://localhost:8088/login/` for the separate admin site. On Windows use `python` instead of `python3`.

Build production with `CODARIS_PRODUCTION=1` (PowerShell: `$env:CODARIS_PRODUCTION = '1'`). Package the gated admin site with `python3 scripts/package-site.py RELEASE_NAME --admin`. The existing main-site deployment script does not deploy the admin site. `deploy/nginx/admin.codaris.conf.example` is an uninstalled configuration template, not a live vhost. DNS, TLS issuance and deployment have not been performed. The admin build must have its own root; never deploy its files over the main site.

## Backend contract required before live access

Sign-in should accept either a permanent membership ID or verified email plus password. Both identifiers are identifiers, not additional authentication factors. Credentials must be verified server-side; then current staff role and account eligibility must be checked. Deny non-staff and suspended accounts with generic responses. Repeat authorization on every application API request, not just login. Staff roles must never come from request fields, URLs, client storage or profile edits.

The application queue should read persisted, access-controlled applications through native C/libpq parameterized queries. No public/static personal data. Final per-role permissions, review state transitions and staff provisioning require a backend design before enabling actions. This template intentionally treats all roles as read-only.

Use host-only secure HttpOnly sessions on the admin origin, CSRF protection, throttling and audit events. Do not pass passwords or session tokens in member-to-admin URLs or broadly share a parent-domain cookie. Role revocation must invalidate access on subsequent requests. Email/password recovery and provider integration remain subject to the membership backend design. No authentication schema, cryptography, TLS or production dependency has been added.
