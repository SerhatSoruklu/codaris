# Security

Report vulnerabilities privately through this repository's GitHub Security tab using **Report a vulnerability**. Do not publish credentials or exploit details in a public issue. If private reporting is unavailable, request a private contact channel without including vulnerability details.

The deployed site is static HTML/CSS with a C17/WebAssembly client. It has no live submission, authentication, payment, or HTTP API. The native C program is a development database health check and must not be exposed as a service.

Security controls include a restrictive Content Security Policy, disabled JavaScript string evaluation in Emscripten, text-only DOM insertion, no inline executable handlers, denied framing and form submission, and an allowlisted deployment artifact. PostgreSQL access stays in the backend; browser assets must never contain connection credentials.

Changes to accounts, payments, uploads, form submission, or APIs require a separate security design. No claim of immunity to XSS, denial of service, or other attacks is made.
