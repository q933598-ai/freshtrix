# Larptrix integration progress

This checklist tracks actual port work on `larptrix-port`. It deliberately distinguishes code that exists from work that is still planned.

## Implemented foundation

- [x] Add an isolated `Larptrix::Api` HTTP client under `Telegram/SourceFiles/larptrix/`.
- [x] Add a transport-independent `SessionModel` that caches current user, friends, directory, groups, chat history and message metadata from server events.
- [x] Add an optional Qt WebSockets transport for authenticated `/ws` with Larptrix session-cookie forwarding, same-origin `Origin`, JSON event parsing, a heartbeat, presence, and open-chat requests.
- [x] Route WebSocket events through `SessionModel`, including directory, groups, chat history, live messages, deletes, reactions, views and presence.
- [x] Normalize and validate an HTTP(S) server base URL; a missing scheme defaults to HTTPS.
- [x] Implement password and access-key login requests to `POST /api/login`.
- [x] Send a device name with login requests.
- [x] Reuse the `QNetworkAccessManager` cookie jar so the server's HTTP-only session cookie is sent on later requests.
- [x] Implement `GET /api/me` and `POST /api/logout`.
- [x] Emit success/failure and authentication-state signals.
- [x] Register the new C++ files in the existing desktop target.
- [x] Add a temporary Larptrix login widget with server URL, email/password and access-key modes, connection/error status, and authenticated-user signal.
- [x] Replace the interim login widget's Nord-blue hard-coded palette with a dark olive/light-green palette inspired by FreshGram. This does not replace the actual FreshGram chat UI.
- [x] Remember the selected server URL between launches; clear password/access-key fields after successful authentication.
- [x] After login, request `GET /api/friends` and show a temporary friends list with display names and usernames. This interim widget is not the final FreshGram chat UI.
- [x] Remove a duplicate `fetchFriends()` declaration from the API header.
- [x] Route the existing MainWindow intro entry point to the Larptrix login widget and include it in focus, resize, and widget-cleanup handling.

## Not implemented yet

- [ ] Replace the existing Telegram intro and MTProto account/session lifecycle.
- [ ] Render real-time users, groups, and chat history using the existing FreshGram UI components and Larptrix data models.
- [ ] Port Matrix-compatible E2E device setup/decryption before rendering message contents or enabling message sending.
- [ ] Replace Telegram data models and history UI with Larptrix users, groups, channels, and messages.
- [ ] Port attachments, Matrix-compatible E2E, and WebRTC calls.
- [ ] Remove unused Telegram/MTProto code, resources, build requirements, and CI secrets after the replacement path is working.

## Protocol notes

- Larptrix login is HTTP cookie-session authentication. Successful `/api/login` and `/api/me` return a `UserInfo` object directly; the account-creation endpoint has a different wrapper.
- `/ws` is a separate authenticated JSON WebSocket. Its messages are defined in the Larptrix `crates/protocol` crate; do not attempt to reuse MTProto for it.
- The login widget is connected to the MainWindow intro entry point and now requests the friends endpoint after authentication, but the app is still an inherited Telegram Desktop codebase and has not been built or live-server tested in this work session.
- After login, the temporary widget displays friends and, when the build provides Qt WebSockets, attempts the authenticated `/ws` connection and requests history on a friend's double-click. At present it only reports event/history counts; it does not display decrypted messages or provide a send box.
- The native E2E implementation is not wired into this client, so the client intentionally does not send message bodies or show server-stored ciphertext as if it were readable text.


## UI porting rule

The goal is to reuse the existing FreshGram/Freshtrix interface and replace Telegram-backed behavior with Larptrix-backed behavior. Do not build a parallel generic UI or make Nord the mandatory visual theme. Keep existing chat-list, conversation, avatar, and settings components wherever their dependencies can be separated from Telegram session/data objects.

## Next implementation slice: reusing FreshGram chat UI

The Larptrix protocol crate currently defines these WebSocket messages:

- Client: `ping`, `open { peer_id }`, `send { peer_id, body, attachment_id / attachment_ids }`, `set_presence`, plus reactions, deletes, call signaling and crypto-resync messages.
- Server: `welcome { user, users }`, `directory { users }`, `groups { groups }`, `chat { peer, history }`, `message { message }`, presence, reactions, deletes, call signaling, crypto-resync, and errors.

`Larptrix::WebSocketClient` now handles transport when Qt WebSockets is available: it maps HTTP(S) to WS(S), forwards the session cookie and same-origin header, parses JSON server events, and sends heartbeat/presence/open-chat events. `SessionModel` now converts these events into reusable client state. CMake enables WebSockets conditionally because some TDesktop Qt bundles do not ship the module. The code has not been built or tested against a live server yet.


## Existing FreshGram UI dependency audit

The existing visual interface cannot yet consume Larptrix JSON directly:

- `MainWindow::setupMain()` still requires `account().sessionExists()` and constructs the original `MainWidget` with a `Window::SessionController`.
- The native dialogs widget also takes a `Window::SessionController`; rows and history are backed by Telegram's `PeerData`, `History`, `HistoryItem`, and `Data::Thread` objects.
- Larptrix now has a separate `SessionModel`; it is not a replacement for those Telegram types. Constructing the original main window widgets without the Telegram session would be unsafe and incorrect.

The UI migration therefore needs an adapter/refactor boundary between Larptrix conversation data and the existing presentation. The goal remains preserving FreshGram's layout and visual components, not introducing a new permanent theme or claiming that the interim login/friends widget is the finished client. Until this boundary exists, the real chat UI and E2E display/send path remain incomplete.
