# Larptrix integration progress

This checklist tracks actual port work on `larptrix-port`. It deliberately distinguishes code that exists from work that is still planned.

## Implemented foundation

- [x] Add an isolated `Larptrix::Api` HTTP client under `Telegram/SourceFiles/larptrix/`.
- [x] Normalize and validate an HTTP(S) server base URL; a missing scheme defaults to HTTPS.
- [x] Implement password and access-key login requests to `POST /api/login`.
- [x] Send a device name with login requests.
- [x] Reuse the `QNetworkAccessManager` cookie jar so the server's HTTP-only session cookie is sent on later requests.
- [x] Implement `GET /api/me` and `POST /api/logout`.
- [x] Emit success/failure and authentication-state signals.
- [x] Register the new C++ files in the existing desktop target.

## Not implemented yet

- [ ] Show a Larptrix server/login screen at application startup.
- [ ] Replace the existing Telegram intro and MTProto account/session lifecycle.
- [ ] Implement the `/ws` client and parse Larptrix server events.
- [ ] Replace Telegram data models and history UI with Larptrix users, groups, channels, and messages.
- [ ] Port attachments, Matrix-compatible E2E, and WebRTC calls.
- [ ] Remove unused Telegram/MTProto code, resources, build requirements, and CI secrets after the replacement path is working.

## Protocol notes

- Larptrix login is HTTP cookie-session authentication. Successful `/api/login` and `/api/me` return a `UserInfo` object directly; the account-creation endpoint has a different wrapper.
- `/ws` is a separate authenticated JSON WebSocket. Its messages are defined in the Larptrix `crates/protocol` crate; do not attempt to reuse MTProto for it.
- The HTTP bridge is an initial integration layer, not yet connected to the visible login UI. No build or live-server test has been claimed for this change.
