# CODARIS Architecture Baseline

## Principle

CODARIS is C-first.

- Browser application logic: C17 compiled to WebAssembly with Emscripten.
- Browser host layer: semantic HTML/CSS presentation and narrow DOM/event glue.
- Server/backend: native C.
- Primary datastore: PostgreSQL.
- PostgreSQL access: `libpq`, PostgreSQL's native C client interface.

## Trust boundaries

```text
Chrome
  |
  | C/Wasm client
  | HTTPS (future)
  v
CODARIS native C API
  |
  | libpq
  v
PostgreSQL
```

The browser never receives PostgreSQL credentials and never connects directly to PostgreSQL.

## Current baseline

Implemented:

1. C/Wasm-owned landing-page data, search, and demo submission state with a semantic DOM host.
2. Native C executable built with CMake.
3. PostgreSQL connection abstraction using libpq.
4. Database health query.
5. Initial migration for users and external profiles.
6. PowerShell build/run/migration helpers.

Not implemented intentionally:

- HTTP server/routing
- authentication
- sessions
- password storage
- LinkedIn ownership verification
- production deployment
- observability
- automated tests

Those should be designed before implementation rather than accumulated ad hoc.

## Long-term rules

- Keep application code in C17 unless a later migration is explicitly decided.
- Prefer stable, boring interfaces over framework churn.
- Add dependencies only when they remove more risk than they add.
- Preserve portability between Windows development and Linux production.
- Treat ownership/lifetime of every allocation and resource as part of the API contract.
- Use parameterized SQL for all user-controlled data.
- Use migrations for schema evolution; never silently mutate production schema.
- Keep secrets out of Git and out of browser-delivered code.

## V1 frontend boundary

The requested page scaffold replaces the blank SDL surface with browser-native semantic HTML and CSS. This gives headings, links, tables, form validation, keyboard access, selection, and responsive layout without recreating browser controls in a canvas. SDL is no longer linked into this client. No new production dependency was added.

Static editorial content lives in `web/pages/*.html`; `web/index.html` is the document template, `web/partials/` holds the shared header/footer, and `web/styles.css` owns layout. Standard-library Python assembles physical directory routes from `web/pages.json` at build time; it adds no browser application logic. C owns the country view model, case-insensitive name/code search, aggregate metrics, country options, and preview outcome. `EM_JS` functions perform synchronous DOM operations only; `web/host.js` forwards input and submit events through `ccall`. Wasm stays alive after `main` for these callbacks. Bridge calls copy C strings immediately, and `ccall` releases its temporary input storage after the synchronous call. No dynamic C allocations are needed.

The public `/ecosystem/organizational-participation/` route is the canonical Coming Soon explanation for future institutional participation. Dashboard links point to its anchored sections instead of duplicating the content or storing organization records. The dashboard disclosure is native button/link markup; `web/host.js` handles hover, keyboard focus, Escape and mobile expansion. Working Groups remain a separate CODARIS-wide concept. The static route builder accepts validated nested path segments for directory routes; this does not add a client-side router or database state.

If Wasm is unavailable, the editorial page remains readable, and interactive controls stay disabled with a runtime message. The table has a labeled keyboard-focusable scroll region; the preview result is announced and focused. All dynamic strings enter the DOM through `textContent`, never HTML parsing.

### Fixtures and future integration

`src/client/demo_data.h` contains invented illustrative figures, not external statistical sources. The reach metric sums every seeded developer estimate; supporters are summed from rows; represented countries count rows with nonzero supporters. The organisation metric is a separate explicit demo fixture. Status labels are illustrative, not evidence of an active local chapter. Replace these fixtures with reviewed, dated, sourced estimates and validated API data before public reporting. Search is ASCII case-insensitive for the current English names and country codes; locale-aware matching needs a future design.

The form is currently gated: a blurred, disabled, inert fieldset sits beneath an accessible Coming soon notice. Wasm initialization never enables it. Prepared native constraints and C-owned messages/counters support name (2–120), email (254 max), HTTPS URLs (2,048 max), and motivation (20–2,000); browser lengths follow HTML UTF-16 semantics. The DOM bridge passes only lengths and validity flags to C. Counters warn at 90 percent and at the limit. Removing the visual overlay alone does not enable submission; the old preview callback now only reports that applications are closed. Personal fields are never copied into C, sent to a service, logged, or put in browser storage. No live supporter counts change. Future submission requires a designed C HTTPS API, privacy/retention policy, server-side input validation and URL normalization, abuse controls, and a deliberate database migration. Profile URLs remain unverified claims.

### Ecosystem assets and claims

Microsoft and LinkedIn appear as informational ecosystem/platform references, without partnership claims. Coupyn is the proposed processing and coordination layer, explicitly not an active integration. The community engagement panel leaves space for a future Microsoft community request. All three names have local SVG symbols. The Microsoft and LinkedIn symbols are vector reproductions; Coupyn is a small-format vector adaptation of user-supplied reference artwork, not the original image file. Replace adaptations with original production assets when available. The CODARIS emblem is original decorative artwork. The globe now uses Natural Earth geographic outlines; see `web/assets/GEOGRAPHY.md` for source, projection, custom regional highlight, and regeneration instructions. CSS-only connection animation includes pause and reduced-motion support. Footer resources have dedicated, noindex placeholder routes.

Both Bash and PowerShell builds compile the same C17 entrypoint with optimization, assemble static routes, and copy CSS, host bridge, and SVG assets. Home and Global Reach alone load the Wasm runtime; DOM bridges tolerate page-specific elements being absent. Both run helpers rebuild before serving. No backend or database behavior changes in this version.
