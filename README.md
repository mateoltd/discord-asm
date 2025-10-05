# discord-asm

Cross-platform **Discord bot framework in Assembly** (x86-64 first; AArch64 later) with a **thin C shim** for networking/TLS/WebSocket/JSON. The public API you write bots against is Assembly-first; the C shim only provides the pragmatic bits (TLS, sockets, timing).

> Built with a tight iterative loop using the **standalone Copilot CLI** (installed via `npm`, not the GitHub CLI extension).

---

## Table of contents

* [Goals](#goals)
* [Status](#status)
* [Architecture](#architecture)
* [Directory Structure](#directory-structure)
* [Prerequisites](#prerequisites)
* [Install & Build](#install--build)

  * [Windows](#windows)
  * [macOS](#macos)
  * [Linux](#linux)
* [Configuration](#configuration)
* [Running the Examples](#running-the-examples)
* [Development Workflow (Copilot CLI)](#development-workflow-copilot-cli)
* [Testing](#testing)
* [ABI & Calling Conventions](#abi--calling-conventions)
* [Docs & ADRs](#docs--adrs)
* [Security Notes](#security-notes)
* [Troubleshooting](#troubleshooting)
* [Roadmap](#roadmap)
* [Contributing](#contributing)
* [License](#license)

---

## Goals

* Minimal, **fast** framework to build Discord bots in Assembly.
* Strong **verifiability**: small iterations, tests, and documentation updates every change.
* **Portable** across Windows, macOS, and Linux.
* Keep crypto/WebSocket/HTTP complexity in a **small, auditable C layer** with a stable ABI callable from Assembly.

## Status

✅ **MVP Implemented**: Core Gateway client with Hello/Identify/Heartbeat protocol support  
✅ **Cross-platform C Shim**: WebSocket, JSON parsing, and timing functions  
✅ **Assembly Gateway Core**: Main event loop and state management in x86-64 Assembly  
✅ **Test Suite**: Unit tests for JSON parsing and heartbeat timing  
🔄 **Dependencies**: Requires libwebsockets + OpenSSL for full build  

**Next**: Add reconnect/resume logic, table-driven event dispatcher, and minimal REST helpers.

---

## Architecture

```
┌──────────────────────┐       ┌──────────────────────────┐
│   Your Bot (ASM)     │       │  discord-asm Core (ASM)  │
│  handlers / router   │◄──────│ gateway loop, dispatch   │
└─────────▲────────────┘       └─────────▲────────────────┘
          │  stable ABI                         │ C ABI
          │                                     │
          │                          ┌──────────┴───────────┐
          │                          │     C Shim (C)       │
          │                          │ ws/tls/json/time     │
          │                          └──────────▲───────────┘
          │                                     │
          ▼                                     ▼
                                        OS sockets + TLS libs
```

* **Assembly**: event loop, heartbeat, dispatcher, user-facing API/macros.
* **C shim**: thin wrappers for WebSocket (WSS), TLS, JSON encode/decode, time/sleep.
* **Vendors**: `libwebsockets` + `OpenSSL` (or mbedTLS), and a tiny JSON lib with a C ABI.

---

## Directory Structure

```
discord-asm/
├─ asm/                     # Assembly sources (x64 first; aarch64 later)
│  ├─ x64/
│  └─ aarch64/
├─ cshim/                   # Minimal C wrappers (ws.c, ssl.c, json.c, time.c)
│  └─ include/
├─ include/                 # Shared ABI headers (structs/opcodes/callconv)
├─ vendor/                  # External deps (optional submodules) 
├─ examples/                # Example bots (echo, ping, slash_echo)
├─ tests/                   # Unit/integration tests + fixtures
├─ scripts/                 # dev tooling (loop, docs, release)
├─ cmake/                   # toolchain & Find*.cmake modules
├─ docs/
│  └─ ADRs/                 # Architecture Decision Records
├─ CMakeLists.txt
├─ README.md                # (this file)
├─ CHANGELOG.md
└─ PROMPT.md                # control prompt for Copilot CLI (separate)
```

---

## Prerequisites

* **Assembler/Tooling**

  * NASM (x86-64)
  * CMake (generates for Ninja/MSBuild/Make)
  * A C/C++ toolchain (MSVC / Clang / GCC)
* **Libraries**

  * OpenSSL (or mbedTLS)
  * libwebsockets (or equivalent)
* **Node.js (for Copilot CLI only)**

  * Node.js ≥ 22 and npm ≥ 10

> We use the **standalone Copilot CLI** (from `npm`). We do **not** use the GitHub CLI extension.

---

## Install & Build

### Windows

1. Install tools (choose one package manager):

```powershell
# Winget
winget install Kitware.CMake
winget install GnuWin32.NASM
winget install Git.Git

# (Optional) Ninja & OpenSSL (if not available on your system)
winget install Ninja-build.Ninja
winget install ShiningLight.OpenSSL
```

2. Clone and configure:

```powershell
git clone https://github.com/mateoltd/discord-asm.git
cd discord-asm
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DOPENSSL_ROOT_DIR="C:\Program Files\OpenSSL-Win64"
cmake --build build --config RelWithDebInfo
```

> If you prefer MSVC generator:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config RelWithDebInfo
```

### macOS

```bash
# Tools & libs
brew install nasm cmake ninja openssl libwebsockets

git clone https://github.com/mateoltd/discord-asm.git
cd discord-asm
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DOPENSSL_ROOT_DIR="$(brew --prefix openssl)"
cmake --build build
```

### Linux (Debian/Ubuntu)

```bash
sudo apt-get update
sudo apt-get install -y build-essential nasm cmake ninja-build libssl-dev libwebsockets-dev

git clone https://github.com/mateoltd/discord-asm.git
cd discord-asm
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```

> If your distro doesn’t ship `libwebsockets-dev`, you can vendor it in `vendor/` and point CMake at it with `-DLWS_ROOT=...`.

---

## Configuration

Provide your Discord Bot Token via environment variable (never commit tokens):

**Windows (PowerShell)**

```powershell
$env:DISCORD_BOT_TOKEN = "your-token-here"
```

**macOS/Linux**

```bash
export DISCORD_BOT_TOKEN="your-token-here"
```

Optional env vars:

* `DISCORD_INTENTS` — comma/pipe separated or integer bitfield (defaults target common non-privileged intents).
* `DISCORD_API_BASE` — override REST base (rarely needed).
* `DISCORD_GATEWAY_URL` — override Gateway URL (for testing).

---

## Running the Examples

After building:

```bash
# Echo example — prints READY event and echoes messages
./build/examples/echo/discord-asm-echo
```

Expected behavior:

* Connects to the Gateway over WSS
* Receives **Hello**, sends **Identify** with your token
* Starts **Heartbeat** at server interval
* Logs **READY**, then echoes messages it sees (according to configured intents)

---

## Development Workflow (Copilot CLI)

We keep an explicit “ask-build-verify-document” loop using the **standalone** Copilot CLI.

1. Install Copilot CLI:

```bash
npm install -g @github/copilot
copilot
# In the chat: /login and follow instructions
```

2. From repo root, start a session:

```text
/model gpt-5
/load PROMPT.md
```

3. Use the **Iteration template** inside `PROMPT.md`:

* PLAN the smallest change
* CODE (let Copilot produce diffs)
* VERIFY (build & run tests)
* DOCS (update ADRs, README sections, CHANGELOG)
* NEXT (print a short TODO list)

> Keep commits small. Each iteration should either pass tests or add a failing test with a clear next step.

---

## Testing

* Unit tests for parsers/heartbeat timing (C test harness or minimal C/ASM glue).
* Fixture-based tests for Gateway payloads in `tests/fixtures/*.json`.
* Integration smoke test that connects to Gateway with a test bot token (opt-in via env var).

Run:

```bash
ctest --test-dir build
```

---

## ABI & Calling Conventions

We target x86-64 first:

* **Windows x64 ABI**: RCX, RDX, R8, R9 for integer args; 32-byte shadow space; callee-saved: RBX, RBP, RDI, RSI, R12–R15.
* **SysV AMD64 (macOS/Linux)**: RDI, RSI, RDX, RCX, R8, R9; red zone; callee-saved: RBX, RBP, R12–R15.

The C shim exposes a small, stable surface (documented in `include/abi.h`) that Assembly calls identically across platforms with thin shims as needed. An ADR explicitly records these rules.

---

## Docs & ADRs

* ADRs live in `docs/ADRs/ADR-XXXX-*.md` (0001: MVP + shim rationale; 0002: calling conventions; 0003: REST surface, etc.).
* `CHANGELOG.md` follows “Keep a Changelog” style with an **Unreleased** section.
* Protocol notes (Gateway opcodes, heartbeat, resume, status codes) live in `docs/protocol-notes.md`.

---

## Security Notes

* **Never** hard-code tokens. Use environment variables or a secrets store.
* Treat logs carefully; do not print tokens or Authorization headers.
* Pin library versions where practical and document updates in ADRs.
* Prefer OpenSSL/mbedTLS from your system package manager or a vetted vendored build.

---

## Troubleshooting

**Build can’t find OpenSSL**

* Windows: pass `-DOPENSSL_ROOT_DIR="C:\Program Files\OpenSSL-Win64"` (or your path).
* macOS: `-DOPENSSL_ROOT_DIR="$(brew --prefix openssl)"`.
* Linux: ensure `libssl-dev` is installed; otherwise vendor it and point CMake to it.

**Link errors with libwebsockets**

* Verify headers/libs are present; on Windows, confirm the correct architecture (x64) and runtime.

**Gateway connect/identify fails**

* Double-check `DISCORD_BOT_TOKEN`.
* Ensure required intents are enabled for the bot in the Discord Developer Portal (and set `DISCORD_INTENTS` accordingly).

**Copilot CLI issues**

* Make sure you installed `@github/copilot` from `npm` and authenticated via `/login`.
* If suggestions reference the GitHub CLI extension, remind Copilot we use the standalone CLI.

---

## Roadmap

* ✅ MVP: Hello/Identify/Heartbeat Gateway client
* ✅ Cross-platform C shim (WebSocket/JSON/timing)  
* ✅ Assembly x64 core with event loop
* ✅ Basic test suite and documentation
* ⏭ Full build system with dependency detection
* ⏭ Reconnect/Resume with backoff & jitter
* ⏭ Table-driven event dispatcher + macro DSL for handlers
* ⏭ Minimal REST helpers (send message)
* ⏭ AArch64 builds (macOS ARM64, Linux ARM64)
* ⏭ CI matrix (Windows/macOS/Linux) + prebuilt artifacts

---

## Contributing

* Open an issue describing the feature/bug.
* Keep PRs small and focused; include tests and ADR/CHANGELOG updates.
* Style: NASM for Assembly; C99+ for shim; CMake for builds.

---

## License

SPDX-License-Identifier: MIT (see `LICENSE`).

---

**Happy hacking!** If you’re using this to learn Assembly, the `examples/` are intentionally small and heavily commented—start there, then peek into the `asm/x64` core.