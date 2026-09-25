# Transactional email source drafts

The historical HTML/text pairs in this directory remain design references and are not copied into the public website. The active C mail renderer is `src/server/mail.c`: responsive CODARIS navy/cyan card, action button, plain-text alternative and footer, following the structure of the Serhat application’s email layout.

Active events: verification, password reset, email-change notification to the former address, and password-change notification. Membership activates automatically after verification; there is no staff approval email. Login accepts either email or membership ID, so recovery does not need to email an ID.

Delivery uses Google Workspace `smtp.gmail.com:587`, mandatory STARTTLS and certificate validation in production, with `admin@coupyn.com` as the authorized sender and CODARIS as display name. Reply-To is contact@codaris.org. A no-reply@codaris.org From address requires verified Google send-as configuration first.

Account transactions enqueue mail durably. The worker decrypts action tokens only for rendering, sends multipart mail through libcurl and removes encrypted payloads on success. The worker retries up to eight times; stable Message-ID and single-use links mitigate duplicate deliveries, but SMTP is at-least-once. Links use the configured origin, never a request Host header. No user content enters message HTML. Secrets, passwords and token URLs are not logged.

See [release runbook](../../docs/RELEASE.md) for local capture testing, production configuration and delivery verification.
