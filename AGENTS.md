# CODARIS repository instructions

## Project identity

CODARIS means Coalition Of Developers Advancing Responsible Intelligent Systems.
The engineering principle is: BUILD. VERIFY. ADVANCE. Trust is engineered.

## Architecture contract

- Application implementation language is C.
- Use C17 for source code unless the repository owner explicitly approves another C standard.
- Browser application code is C compiled to WebAssembly with Emscripten.
- Keep HTML/JavaScript limited to unavoidable browser host/bootstrap glue. Do not migrate application logic into JavaScript.
- Do not introduce CSS or product design work until explicitly requested.
- Native backend code is C.
- PostgreSQL is the authoritative datastore.
- Use PostgreSQL through libpq from the backend only.
- Never expose database credentials to browser/Wasm code.
- Development supports Linux/WSL + Bash + VS Code, alongside Windows 11 + PowerShell. Keep both platforms supported.

## Security and correctness

- Never commit passwords, tokens, connection strings containing credentials, private keys, or other secrets.
- Never concatenate user-controlled values into SQL. Use libpq parameterized query APIs such as PQexecParams/PQsendQueryParams.
- Treat external profile URLs as claims until ownership is separately verified.
- Validate and normalize external URLs server-side before persistence.
- Be explicit about ownership and lifetime of memory, handles, sockets, database results, and file descriptors.
- Check every fallible allocation and external call.
- Do not write custom cryptographic primitives.
- Do not write a custom TLS stack.

## Database evolution

- Applied migrations are immutable.
- Add sequential migration files for schema changes.
- Prefer constraints in PostgreSQL when they protect durable invariants.
- Authentication/password schema is intentionally absent from the initial migration. Design it before adding it.

## Dependency policy

- Do not add a production dependency merely for convenience.
- Before adding one, state what problem it solves, security/maintenance implications, license, and why the standard library or an existing dependency is insufficient.
- Prefer mature C libraries with clear maintenance and security records.

## Change discipline

- Understand the existing flow before editing it.
- Keep changes small and reviewable.
- Preserve existing architecture unless there is a concrete reason to change it.
- Add or update documentation when architecture or operational behavior changes.
- Never claim a build, test, migration, or command succeeded unless it was actually executed and its result was observed.
- When a task is ambiguous in a way that affects architecture/security/data, ask before guessing.
