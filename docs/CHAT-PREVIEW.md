# Country chat preview

The development member dashboard has a Community chat tab. Select one of 50 country rooms before entering. This is a single-page interactive preview, not a socket service: messages, attachments, reports and blocks never leave the page, and everything resets on navigation/reload. Sample members are explicitly fictional. Production's existing dashboard gate removes the entire chat interface.

Display names come from the applied profile preview; membership IDs are never read by chat rendering or input transport. Avatars use the local profile canvas with the geometric fallback. Room choice is voluntary, not verified residence. Each room retains its own messages during the page visit; the preview holds at most 100 sent messages in total. Example-member blocks apply across rooms.

## Implementation

`src/shared/chat_preview.h` contains C17 state and policy. `src/client/chat.h` connects it to the existing Wasm client. `web/host.js` supplies DOM rendering, input transport, local file decoding and canvas thumbnails. No production dependency, Socket.IO server, browser storage or database schema is introduced.

C applies a two-second send interval and a five-message / 30-second rolling limit across rooms, recent-message duplicate checks, repeated-character and all-capital spam checks, and a small English offensive-language example list with limited punctuation/leet normalization. Text uses a 2,000-byte UTF-8 limit. These checks deliberately do not claim comprehensive abuse detection, multilingual moderation or a security boundary. Browser state and timestamps are user-controlled.

Messages and names are rendered with textContent, never as HTML. Emoji are inserted into the textarea. PNG/JPEG/WebP, PDF and UTF-8 TXT are allowed for local selection, with matching extension/MIME and a 5 MiB limit. Images must decode and be at most 4096 pixels per side; image bitmaps are closed after rendering bounded thumbnails. PDF headers and UTF-8 decoding are basic preview checks, not malware scanning. Non-image attachments show filenames only, with no execution or download. Pending asynchronous selections are cancelled when switching rooms or removing an attachment. Files are not uploaded, scanned, transmitted or retained after reload.

Reports require a reason and clearly state no moderator receives them. Blocking and reporting operate only on the fictional example actors. No real member moderation or identity verification is implemented.

## Validation

Native policy tests: `cc -std=c17 -Wall -Wextra -Wpedantic tests/chat-preview.c -o /tmp/codaris-chat-test && /tmp/codaris-chat-test` (on Windows use a C17 compiler and a local executable output path). Linux checks also ran AddressSanitizer and UndefinedBehaviorSanitizer.

Run the client build, `python3 tests/check-site.py` and `python3 tests/check-membership-build.py`. Browser checks cover all 50 options, entry gating, room history separation, rate limits, offensive examples, emoji, reporting/blocking, ID privacy, display names and avatars, attachments and malformed files, safe text rendering, keyboard tab navigation, mobile overflow and reload reset.

## Before real multi-user chat

Design native C authenticated HTTP/WebSocket endpoints and PostgreSQL persistence first. The server must derive identity and permissions from authenticated sessions, validate room membership, enforce rate limits and moderation, and authorize attachment access. Real uploads need server validation, bounded decoding, malware scanning and isolated storage/delivery. Reporting requires a staffed workflow, audit records and retention rules. Client-side filtering remains only feedback; no part of this preview grants production trust.
