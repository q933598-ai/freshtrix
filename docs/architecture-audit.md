# Freshtrix architecture audit — Phase 0

Branch: `larptrix-port`

This audit is based on the tracked source tree and Larptrix's current public source. It identifies the first investigation seams; it is not a claim that the application has been built or run.

## Key finding

FreshGram is a full Telegram Desktop C++/Qt application, not a thin skin over an independent messenger UI. The repository has a large Telegram data/session/API architecture and generated Telegram API schema. Larptrix is a separate Axum HTTP + WebSocket service with JSON messages and cookie sessions. There is no compatible protocol switch or single server URL replacement.

**Strategy:** preserve the visual framework and portable UI pieces, then introduce a distinct Larptrix backend adapter and replace Telegram-backed data flow incrementally. Do not mass-delete the MTProto tree at the start: many high-level UI models depend on it.

## FreshGram areas to trace

| Area | Initial source paths | What to determine |
|---|---|---|
| Application startup | `Telegram/SourceFiles/main.cpp`, `Telegram/SourceFiles/window/main_window.cpp` | Startup sequence, account selection, main-window creation |
| Intro / login | `Telegram/SourceFiles/intro/intro_widget.cpp`, `intro_phone.cpp`, `intro_code.cpp`, `intro_signup.cpp`, `intro_qr.cpp` | How Telegram phone/code authorization is entered and how to replace it with Larptrix access-key or email/password login |
| Session and authorization state | `Telegram/SourceFiles/data/data_authorization.h`, `Telegram/SourceFiles/data/data_session.cpp`, `Telegram/SourceFiles/main/main_session.cpp` | Session ownership, current-user identity, lifecycle assumptions |
| Telegram API facade | `Telegram/SourceFiles/apiwrap.h`, `Telegram/SourceFiles/apiwrap.cpp`, `Telegram/SourceFiles/api/api_authorizations.cpp` | Which UI-facing operations call Telegram RPCs and whether seams can be introduced |
| MTProto transport | `Telegram/SourceFiles/mtproto/`, `Telegram/SourceFiles/mtproto/mtp_instance.cpp`, `Telegram/SourceFiles/mtproto/facade.cpp` | Telegram-only transport and initialization; not reusable as Larptrix protocol |
| Chat list | `Telegram/SourceFiles/dialogs/dialogs_widget.cpp` and the dialogs model directories | Dialog ordering, filters, list item dependencies |
| Message history/composer | `Telegram/SourceFiles/history/history_widget.cpp`, `Telegram/SourceFiles/history/history.cpp` | Message models, pagination, send/edit/delete paths, media assumptions |
| Build configuration | root `CMakeLists.txt`, `Telegram/CMakeLists.txt`, `Telegram/cmake/` | Generated API dependencies and the minimum buildable configuration |

Paths above are entry points, not a complete dependency map. Follow includes, signal/slot connections, and model ownership before changing them.

## Larptrix contract discovered

The current Larptrix source defines a Rust/Axum server and JSON WebSocket protocol.

### HTTP routes to integrate

- Authentication/session: `POST /api/register`, `POST /api/register/password`, `POST /api/login`, `POST /api/logout`, `GET /api/me`
- Session management: `GET /api/me/sessions`, `DELETE /api/me/sessions/{session_id}`
- Friends and user search: `GET /api/friends`, `POST /api/friends/{id}`, `POST /api/friends/{id}/accept`, `DELETE /api/friends/{id}`, `GET /api/users/search`, `GET /api/users/{id}/profile`
- Groups/channels: `GET/POST /api/groups`, `POST /api/groups/{id}/members`, group settings/avatar/banner routes, `POST /api/channels`, channel admin/settings routes
- Attachments/profile: `POST /api/upload`, `GET /api/attachments/{id}`, `POST /api/me/avatar`, `POST /api/me/banner`
- Crypto devices and Matrix-compatible E2E support: `/api/me/crypto-device*`, `/api/me/matrix-devices/*`, `/api/users/{id}/crypto-devices`, `/api/matrix/*`
- Calls: `GET /api/rtc-config`

The server URL must be configurable. For HTTPS deployments, the client must retain the base URL and use the matching secure WebSocket scheme. Authentication relies on the Larptrix HTTP session cookie; never persist raw passwords/access keys in logs.

### WebSocket protocol

The shared Rust enum in `larptix/crates/protocol/src/lib.rs` defines client messages including `Ping`, `Open`, `Send`, `Delete`, `SetPresence`, `CallSignal`, crypto resynchronization, `React`, and `ViewMessage`. Server events include `Welcome`, `Directory`, `Groups`, `Chat`, `Message`, deletion/reaction/view updates, `Presence`, call state/signaling, Matrix to-device events, and `Error`.

Do not assume the C++ client can use the same protocol without an adapter: define typed C++ DTOs and test JSON serialization against the Rust definitions.

## Proposed C++ module boundary

Create a new module, initially isolated from Telegram data models, e.g.:

- `Telegram/SourceFiles/larptrix/larptrix_api_client.h/.cpp` — HTTP requests, base URL, status/error mapping.
- `Telegram/SourceFiles/larptrix/larptrix_session.h/.cpp` — signed-in user, session state, logout and cookie handling.
- `Telegram/SourceFiles/larptrix/larptrix_ws_client.h/.cpp` — WebSocket lifecycle, ping/pong, reconnect, typed events.
- `Telegram/SourceFiles/larptrix/larptrix_protocol.h` — typed protocol DTOs and serialization.
- Tests for URL construction, login/error responses, and representative client/server WebSocket JSON messages.

These names are a proposal, not existing files. Before adding them, inspect the project's Qt/network conventions, threading rules, CMake source registration, and tests.

## Compatibility risks

1. Telegram dialogs/messages/users are not shaped like Larptrix's users, friends, groups/channels, and encrypted message payloads.
2. Larptrix's WebSocket is an event relay with an explicit `Open` request for history; it is not Telegram's update stream.
3. Larptrix E2E encryption currently uses its existing client-side crypto and Matrix-style device endpoints. The C++ client must interoperate with that exact format or coordinate a deliberate shared crypto library; a fresh implementation could break cross-client decryption.
4. Larptrix calls use WebRTC signaling over the WebSocket plus ICE config. Telegram call UI and Telegram's call protocol cannot be reused as a backend.
5. Telegram-specific features such as phone numbers, Premium, channels semantics, secret chats, deleted-message history, and Telegram cloud settings must be removed or redefined, not simply left visible but nonfunctional.
6. Full source includes Git submodules and generated components; a tree listing alone cannot verify a successful build. Initialize submodules and establish a reproducible baseline before implementation changes.

## Ordered next steps

1. Inspect exact authorization and startup call paths in `main.cpp`, intro widgets, and session classes.
2. Inspect CMake conventions and add a small, isolated Larptrix HTTP/WebSocket proof of concept only after the baseline build is reproducible.
3. Verify login against a test Larptrix server; then add friend/chat listing and message flow.
4. Replace Telegram-facing product flows gradually and keep every step in this branch.
5. Add a CI build/test workflow before considering a pull request into `main`.

## Current status

- [x] Created the `larptrix-port` branch.
- [x] Documented the Larptrix-only product decision and port phases.
- [x] Identified primary FreshGram and Larptrix protocol entry points.
- [ ] Reproducible FreshGram baseline build verified.
- [ ] C++ Larptrix HTTP/WebSocket adapter implemented.
- [ ] Login and messaging tested against a Larptrix server.
