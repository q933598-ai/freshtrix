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
- [x] Add a centered, FreshGram-inspired Larptrix login widget with server URL, email/password and access-key modes, connection/error status, and authenticated-user signal.
- [x] Remember the selected server URL between launches; clear password/access-key fields after successful authentication.
- [x] After login, request `GET /api/friends` and render returned friends in a small Nord-styled list, showing display names and usernames when available.
- [x] Remove a duplicate `fetchFriends()` declaration from the API header.
- [x] Route the existing MainWindow intro entry point to the Larptrix login widget and include it in focus, resize, and widget-cleanup handling.

## Not implemented yet

- [ ] Replace the existing Telegram intro and MTProto account/session lifecycle.
- [ ] Implement the `/ws` client and parse Larptrix server events; this is required for actual conversations and live updates.
- [ ] Replace Telegram data models and history UI with Larptrix users, groups, channels, and messages.
- [ ] Port attachments, Matrix-compatible E2E, and WebRTC calls.
- [ ] Remove unused Telegram/MTProto code, resources, build requirements, and CI secrets after the replacement path is working.

## Protocol notes

- Larptrix login is HTTP cookie-session authentication. Successful `/api/login` and `/api/me` return a `UserInfo` object directly; the account-creation endpoint has a different wrapper.
- `/ws` is a separate authenticated JSON WebSocket. Its messages are defined in the Larptrix `crates/protocol` crate; do not attempt to reuse MTProto for it.
- The login widget is connected to the MainWindow intro entry point and now requests the friends endpoint after authentication, but the app is still an inherited Telegram Desktop codebase and has not been built or live-server tested in this work session.
- A successful login currently confirms authentication and displays the friends returned by `/api/friends`; it does not yet transition into a Larptrix chat interface. The next functional milestone is checking the Qt WebSocket dependency path, then implementing the authenticated `/ws` connection and a minimal users/chat view.


## UI porting rule

The goal is to reuse the existing FreshGram/Freshtrix interface and replace Telegram-backed behavior with Larptrix-backed behavior. Do not build a parallel generic UI or make Nord the mandatory visual theme. Keep existing chat-list, conversation, avatar, and settings components wherever their dependencies can be separated from Telegram session/data objects.

## Next implementation slice: live chat transport

The Larptrix protocol crate currently defines these WebSocket messages:

- Client: `ping`, `open { peer_id }`, `send { peer_id, body, attachment_id / attachment_ids }`, `set_presence`, plus reactions, deletes, call signaling and crypto-resync messages.
- Server: `welcome { user, users }`, `directory { users }`, `groups { groups }`, `chat { peer, history }`, `message { message }`, presence, reactions, deletes, call signaling, crypto-resync, and errors.

The client must keep the authenticated HTTP cookie session for the WebSocket handshake, map the server URL from HTTP(S) to WS(S), and only expose sending after the existing Larptrix E2E flow is integrated. Do not send plaintext messages as a shortcut. The current Freshtrix API bridge has no WebSocket transport yet, and Qt WebSockets availability/linkage has not been confirmed.
