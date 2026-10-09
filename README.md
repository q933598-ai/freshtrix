# Freshtrix

**Freshtrix is an experimental desktop-client port intended to work exclusively with Larptrix. It is not a working Larptrix client yet.**

The project starts from [freshGram](https://github.com/Snowy-Fluffy/freshGram), a Telegram Desktop fork combining the Material Design appearance of [materialgram](https://github.com/kukuruzka165/materialgram) with features inspired by [AyuGram Desktop](https://github.com/AyuGram/AyuGramDesktop). Freshtrix aims to preserve suitable UI and customization work while replacing Telegram-specific account, network, chat, media, and call flows with the APIs and protocol of [Larptrix](https://github.com/q933598-ai/larptix).

## Product direction

- **Larptrix only.** No Telegram login, Telegram accounts, MTProto backend, or Telegram fallback in the finished product.
- **Keep the useful UI.** Retain FreshGram's Material-inspired design and selected customization features where they fit Larptrix.
- **Use Larptrix's existing protocol.** The current Larptrix server exposes HTTP endpoints and an authenticated JSON WebSocket protocol; it is not MTProto.
- **Port in small, reviewable steps.** First map the existing architecture and establish a reproducible build, then implement authentication and core messaging before encryption, calls, and additional customization.

## Current status

This is an architecture and integration experiment. The current source tree is still predominantly Telegram Desktop/FreshGram code. It has **not** yet been converted to a Larptrix-only application, and a successful build or end-to-end Larptrix login has not yet been verified.

See:

- [Architecture audit and source map](docs/architecture-audit.md)
- [Larptrix-only port plan](docs/larptrix-port-plan.md)
- [Nix flake](flake.nix)

Development work is currently happening on the `larptrix-port` branch so that the `main` branch remains untouched.

## NixOS

The flake provides a development shell with common Linux build dependencies and a launcher for an already-built checkout:

```bash
git clone --recurse-submodules --branch larptrix-port https://github.com/q933598-ai/freshtrix.git
cd freshtrix
nix develop
```

After the app has been built, launch it from the repository with:

```bash
nix run
```

The launcher checks the usual upstream output paths. If no executable exists yet, it prints the next steps instead of pretending the app was built. The inherited upstream build is large and currently requires the upstream build process; see [build baseline](docs/build-baseline.md).

**Important:** this Nix setup is an initial development/launch scaffold, not yet a self-contained package derivation. The current source still builds the FreshGram/Telegram-based application, not a usable Larptrix client. The Nix setup does not remove Telegram/MTProto code or make Larptrix login work.

## Upstream and licensing

- Original FreshGram source: [Snowy-Fluffy/freshGram](https://github.com/Snowy-Fluffy/freshGram)
- Larptrix server and protocol: [q933598-ai/larptix](https://github.com/q933598-ai/larptix)
- FreshGram identifies its source as GPLv3 with an OpenSSL exception. Preserve upstream copyright notices, the repository's `LICENSE`, applicable dependency licenses, and attribution when modifying or redistributing code. Review the actual license files before publishing binaries.

Freshtrix is an independent, experimental project and is not an official release or endorsement of FreshGram, materialgram, AyuGram, ExteraGram, Telegram, or Larptrix.

## Build notes

At this stage, upstream build instructions build the underlying FreshGram/Telegram Desktop application, **not a working Larptrix client**. Do not add real Larptrix credentials or expect Larptrix login to work until the port is implemented. See the [upstream build documentation](https://github.com/Snowy-Fluffy/freshGram#build-instructions) for baseline build prerequisites.
