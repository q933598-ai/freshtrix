# Freshtrix: Larptrix-only port plan

## Product decision

Freshtrix is intended to connect **only to Larptrix**. Telegram accounts, Telegram login, Telegram cloud chats, MTProto networking, and Telegram API credentials are not product features. The FreshGram codebase is the starting point for its desktop UI and useful local/client-side components, not a backend to keep alongside Larptrix.

This is an architecture plan, not a claim that the port is already implemented.

## What we can reuse

- Desktop UI and interaction patterns from FreshGram / Telegram Desktop, after checking dependencies and licensing.
- Suitable local features and customization ideas inspired by MaterialGram, AyuGram, and ExteraGram, implemented only where they make sense for Larptrix.
- Existing media viewer, composer, theme, and desktop integration components where they can be separated from Telegram-specific data and services.

## What must be replaced or removed from the product

- Telegram sign-in, phone-number authorization, Telegram API ID/hash setup, and MTProto sessions.
- Telegram cloud contacts, dialogs, channels, message history, uploads, and update/event handling.
- Any feature that requires Telegram servers or Telegram-specific account capabilities.
- Telegram-specific wording, branding, setup instructions, and update endpoints in Freshtrix-facing UI and packaging.

Do not bulk-delete Telegram-derived framework code: many UI components are tightly integrated with the underlying Telegram Desktop architecture. First isolate and replace product-facing network/authentication paths, then remove unused Telegram-only paths safely. Preserve upstream license notices and required attribution.

## Larptrix integration facts

The current Larptrix repository exposes a web API and a WebSocket event protocol; it is not MTProto. The desktop client must therefore use a dedicated Larptrix adapter rather than trying to point Telegram networking at a Larptrix URL.

Observed API surface includes:

- Authentication/session: `/api/register`, `/api/login`, `/api/logout`, `/api/me`
- Friends and discovery: `/api/friends`, `/api/users/search`, `/api/users/{id}/profile`
- Groups and channels: `/api/groups`, `/api/channels`, associated member/settings/avatar routes
- Media: `/api/upload`, `/api/attachments/{id}`, profile/group avatar and banner routes
- Encryption/device management: `/api/me/crypto-devices`, `/api/users/{id}/crypto-devices`, and `/api/matrix/*` routes
- Call configuration: `/api/rtc-config`
- Real-time messaging: authenticated WebSocket at `/ws`, using the JSON protocol defined in Larptrix's `crates/protocol/src/lib.rs`

Authentication differs from Telegram: Larptrix currently supports an access key and a legacy email/password flow, with cookie-based HTTP sessions. The desktop integration must handle session persistence securely and must not log credentials or E2E recovery material.

## Implementation phases

### Phase 0 — map the architecture (current)

1. Identify application startup, account authorization, networking/session state, dialogs, message history, media upload/download, notifications, and call signaling.
2. Trace each area to Telegram-specific APIs or shared UI code.
3. Document the mapping to Larptrix endpoints and WebSocket events before changing behavior.
4. Confirm submodules and build prerequisites; record a reproducible Linux build.

**Exit condition:** a file-level map of the main Telegram-dependent seams and a tested baseline build.

### Phase 1 — isolate the backend

1. Introduce a small Larptrix client/service layer for HTTP, session handling, WebSocket connection, and protocol serialization.
2. Add configurable server URL and a first-run server URL screen.
3. Implement Larptrix login/session validation and logout without starting Telegram authorization.
4. Keep UI changes minimal until authentication and reconnection behavior are understood.

**Exit condition:** the app can sign in to a Larptrix server and receive authenticated real-time events without requiring Telegram API credentials.

### Phase 2 — core messenger

1. Map Larptrix users/friends and groups/channels into UI models.
2. Implement chat opening/history, incoming messages, sending, deletion, reactions, and presence.
3. Implement encrypted attachment upload/download and media rendering.
4. Handle reconnects, errors, empty states, and unsupported server features explicitly.

**Exit condition:** two Larptrix accounts can exchange encrypted messages and supported attachments through Freshtrix.

### Phase 3 — encryption and calls

1. Integrate the existing Larptrix E2E device/key flows; never invent a second incompatible crypto protocol.
2. Handle device registration, key backup/recovery UX, and multi-device limitations.
3. Integrate WebRTC signaling and ICE configuration through Larptrix.
4. Test call setup, hangup, reconnect, permissions, and screen sharing across supported platforms.

**Exit condition:** documented E2E and call flows work against the current Larptrix server with clear failure states.

### Phase 4 — customization and polish

1. Reintroduce appropriate FreshGram customization and useful AyuGram/ExteraGram-inspired features.
2. Remove Telegram-only product screens, API credential setup, and dead menu items.
3. Rename app-facing strings, desktop IDs, icons, packaging metadata, and updater configuration to Freshtrix.
4. Add tests and CI checks for the Larptrix adapter and protocol mapping.

**Exit condition:** a Larptrix-only desktop client with no Telegram account/backend fallback.

## Non-goals for the first milestone

- Supporting Telegram alongside Larptrix.
- Reimplementing the Larptrix server in C++.
- Replacing Larptrix's existing encryption with a new protocol.
- Promising full feature parity with Telegram, AyuGram, or ExteraGram before the core port is stable.

## Immediate next task

Complete Phase 0 by tracing FreshGram's startup and authorization/networking entry points, then create a file-by-file migration map. Do not make a large, unreviewed search-and-replace across Telegram Desktop's source.
