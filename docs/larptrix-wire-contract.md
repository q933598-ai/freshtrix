# Larptrix client wire contract (0.2)

This file captures the server protocol the desktop port must implement. The source of truth is the Larptrix server repository, especially `crates/protocol/src/lib.rs`; if this note and the server disagree, update the note from the server.

## Connection model

- Authentication is ordinary HTTP with a server-managed cookie session.
- The chat event stream is a WebSocket at `/ws`; it is not a Telegram MTProto connection.
- For an HTTPS base URL, the WebSocket URL must use `wss://`; for HTTP, use `ws://`.
- The client should keep cookies in a persistent Qt network cookie jar for requests and the WebSocket handshake. Never put a password or cookie in logs.
- Store the chosen server base URL as user configuration, normalize its trailing slash, and reject non-HTTP(S) schemes.

## WebSocket client messages

The JSON envelope is tagged by `type`, with snake_case names. Current variants include:

- `{"type":"ping"}`
- `{"type":"open","peer_id":"..."}`
- `{"type":"send","peer_id":"...","body":"...","attachment_id":null,"attachment_ids":[]}`
- `{"type":"delete","peer_id":"...","message_id":"..."}`
- `{"type":"set_presence","status":"online"}`
- `{"type":"call_signal","peer_id":"...","kind":"...","payload":{}}`
- `{"type":"react","peer_id":"...","message_id":"...","emoji":"...","add":true}`
- `{"type":"view_message","peer_id":"...","message_id":"..."}`

The protocol also has crypto-recovery messages (`crypto_resync`, `crypto_resync_response`, and `crypto_resync_response_ack`). These must not be silently dropped when E2E is connected.

## Server event families

The client should parse server messages by their `type` field and handle at least:

- Initial state: `welcome`, `directory`, `groups`.
- Chat: `chat`, `message`, `message_deleted`, `message_reaction`, `message_view_update`.
- Presence: `presence`.
- Calls: `group_call_state`, `call_signal`.
- E2E: `crypto_resync`, `crypto_resync_response`, `matrix_to_device`.
- Errors and heartbeat: `error`, `pong`.

Important payload structures include `UserInfo`, `GroupInfo`, `ChatMessage`, `AttachmentInfo`, and `ReactionSummary`. Fields include stable string IDs, display names/usernames, online state, optional avatar URLs, friend/message policy, E2E-enabled state, group/channel metadata, message timestamps, attachment metadata, reactions, and view counts. Preserve unknown fields when practical to allow forward compatibility.

## HTTP API groups

The server currently exposes endpoints for:

- Registration, login, logout, current user and session management.
- User search/profile and friend requests.
- Group/channel management and membership.
- Uploads, attachment retrieval, avatars and banners.
- E2E device registration, Matrix-compatible device/to-device operations.
- RTC configuration.

Before implementing a request, inspect the current server route handler for the exact JSON payload, status codes, and error response. Do not infer a request body from the route name alone.

## Implementation order

1. Build a `LarptrixApiClient` around `QNetworkAccessManager`, a cookie jar, JSON request/response handling, and structured errors.
2. Build a separate WebSocket wrapper with reconnect/backoff and typed event parsing.
3. Add a small session model to own the base URL, authenticated user, and socket lifecycle.
4. Connect the sign-in screen and chat UI to these interfaces.
5. Only then replace Telegram-specific navigation/data providers, and remove MTProto once no longer referenced.

## Current status

This is a protocol design note, not an implementation. HTTP login, WebSocket authentication, E2E compatibility, and call signaling have not yet been tested from Freshtrix.
