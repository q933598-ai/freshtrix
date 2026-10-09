# Build baseline for the Larptrix port

This document records the build path found in the imported FreshGram repository. It is a baseline for the existing Telegram-based application, **not proof that a Larptrix client builds or runs**.

## What the current Linux CI does

The workflow at `.github/workflows/build.yml`:

1. Checks out the repository with recursive submodules.
2. Requires the GitHub Actions secrets `API_ID` and `API_HASH`.
3. Pulls the prebuilt `freshgram-build-env:latest` container from GHCR.
4. Runs `Telegram/build/docker/centos_env/build.sh` in that container.
5. Configures a Release build with CMake and the existing Telegram API credentials.

The build is intentionally split into multiple timed stages and uses ccache. It is a heavyweight C++/Qt build, not a quick smoke test. The workflow currently still targets the original Telegram/MTProto application.

## Before attempting a local build

- Clone with submodules: `git clone --recurse-submodules https://github.com/q933598-ai/freshtrix.git`.
- Check out the port branch: `git switch larptrix-port`.
- Ensure the submodules are initialized: `git submodule update --init --recursive`.
- Read `Telegram/build/README.md` and the platform-specific instructions in `Telegram/build/` before installing packages.
- Use the repository's documented build script rather than inventing a separate CMake invocation. The CI invokes `Telegram/build/docker/centos_env/build.sh` with Release configuration and explicit CMake options.
- Do not commit Telegram API credentials or put them into a Larptrix client. The current workflow's `API_ID`/`API_HASH` requirement belongs to the inherited Telegram build and must be removed once the app no longer uses Telegram's network stack.

## Porting implications

The current top-level CMake project creates the `Telegram` executable and `Telegram/CMakeLists.txt` explicitly links `tdesktop::td_mtproto`, `tdesktop::td_tde2e`, and other Telegram-oriented components. New Larptrix source files will not be compiled just because they exist: they must be registered in the target's source list and linked to the right Qt modules.

Do not remove MTProto dependencies in one sweeping change. First establish a known build, introduce the Larptrix HTTP/WebSocket layer behind a small boundary, then migrate login and session flow. Only after the application no longer references Telegram API functionality should the Telegram-specific dependencies and CI credentials be removed.

## Verification status

- Repository/workflow files inspected: yes.
- Baseline build run in this working session: no.
- Larptrix authentication or chat connection verified: no.
- Existing CI assumed to be Larptrix-ready: no.
