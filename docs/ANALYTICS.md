# Optional Google Analytics

The production public information site uses Google Analytics 4 measurement ID
`G-8P0VYMWP0J`. No Google script, collection request or Analytics cookie is
created before a visitor explicitly allows analytics, including when they reject
it. This uses [Google's basic consent approach](https://developers.google.com/tag-platform/security/guides/consent),
with advertising consent kept denied. There is no preconnect or preload to Google.

The consent controller is C17 in `src/client/analytics.c`, compiled separately to
`analytics-consent.js` and `analytics-consent.wasm` by both build entry points.
Its JavaScript bridges handle browser storage, DOM events and Google's tag API.
The standalone module lets static pages offer consent without loading the
membership runtime. The UI uses existing styles; no CSS is added.

The consent runtime is modularized to keep its Emscripten state separate from
the membership runtime. `web/analytics-bootstrap.js` is appended to the generated
consent loader and starts that module, leaving Google's loading decision to C.

`scripts/build-pages.py` inserts `web/partials/analytics.html` only for production
builds of `https://codaris.org` on indexable public routes, excluding joining and
contact forms. Account/member routes, credentials, staff pages, previews and
error pages have no consent module or Google tag. The runtime also checks the
HTTPS production hostname and development marker before enabling controls.
The footer's Analytics choices link opens the privacy page control in production.

Consent is a versioned localStorage receipt containing an allow/reject value and
timestamp. C rejects missing, malformed, future-dated and expired receipts.
Receipts expire after 180 days. Tabs react to storage changes, and restored or
refocused pages recheck expiry. Storage failure never grants new permission.
Withdrawal saves rejection, sets Google's disable flag, removes this property's
Analytics cookies and reloads to unload Google completely. If storage cannot
save withdrawal, collection is disabled on that page and the UI explains how to
clear the old permission from browser data. Withdrawal cannot recall requests
already sent or delete data Google received.

The tag uses host-only cookies with a 180-day lifetime and no automatic renewal.
The configured page URL omits query strings and fragments; the referrer is empty.
No account IDs or form values are supplied. Google signals and ad personalization
are disabled. In the property's web stream, **turn off Enhanced measurement**
before deployment to prevent Google-controlled automatic event collection
(including outbound link URLs) beyond the configured page views. Keep Google
signals, user-provided data collection and advertising integrations disabled.
Review event retention and account data-sharing settings in Analytics; those
external settings are not controlled by this repository.

Google's hosted tag is a production dependency because the requested property
needs Google's supported browser collection API; C and the standard library do
not provide that service. It executes third-party code and Google can update it
independently of our releases. The hosted software/service is governed by
[Google Analytics terms](https://marketingplatform.google.com/about/analytics/terms/us/),
not a vendored open-source dependency. Google's developer code samples are
Apache 2.0. The CSP allows the tag and collection origins documented in
[Google's CSP guide](https://developers.google.com/tag-platform/security/guides/csp),
plus the narrowly scoped `codaris-analytics` Trusted Types policy for the fixed
tag URL and Google's `goog#html` policy. Inline scripts and JavaScript eval
remain disallowed. Recheck compatibility when Google's hosted code changes.

Before the first deployment, reinstall the reviewed server deployment helper so
its allowlist includes both consent artifacts, as described in `DEPLOYMENT.md`.
Build the production client, run `tests/check-site.py`, and validate Nginx with
`scripts/check-nginx.sh`. In a browser, check fresh, rejected, accepted, restored,
expired and withdrawn choices; intercept collection during local checks to
avoid polluting production reports. After deployment, allow analytics on an
eligible page and check the property's Realtime report. Browser blockers may
prevent collection even after consent.
