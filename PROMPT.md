# PROMPT.md — Control prompt for Copilot CLI

## Objective
Design and implement a cross-platform **Discord bot framework in Assembly** (x86-64 first; AArch64 later) with a thin C shim for networking/TLS/WebSocket. Emphasis: small iterative loops, verifiable increments, and first-class documentation (ADRs, README, CHANGELOG) each step.

## Constraints
- Use **NASM** (x86-64). Toolchain: CMake + Ninja/MSBuild/make.
- Networking/TLS/WebSocket/JSON handled via minimal **C shim** that calls **libwebsockets** + **OpenSSL** (or mbedTLS). Keep the public API Assembly-first.
- MUST work on **Windows**, **macOS**, and **Linux**. Provide build notes per platform.
- No GitHub CLI extension. Use the **standalone `copilot` CLI**.
- Model preference: `/model gpt-5` unless told otherwise.

## Non-Goals (for now)
- No pure-ASM TLS stack.
- No bots that need privileged intents beyond public examples.

## Deliverables every iteration
1) Code changes (scaffold or features).
2) **Documentation updates**:
   - `docs/ADRs/ADR-XXXX-*.md` (Architecture Decision Records).
   - `README.md` quickstart + platform build steps.
   - `CHANGELOG.md` (Keep a Changelog style).
3) **Tests** (unit/fixture/integration) or a note explaining gaps and the next test to add.

## Initial repository plan
- `asm/`             — Assembly sources.
- `cshim/`           — Minimal C wrappers: `ws.c`, `ssl.c`, `time.c`, headers in `cshim/include`.
- `include/`         — Shared ABI headers (struct layouts, enums/opcodes).
- `vendor/`          — libwebsockets + OpenSSL (pkg-config or submodules).
- `examples/`        — `echo/`, `ping/`, `slash_echo/`.
- `tests/`           — Fixtures and basic harness (C or Python runner).
- `scripts/`         — `copilot-loop.*`, `make-docs.*`, `release.*`.
- `docs/`            — ADRs + protocol notes.
- `CMakeLists.txt`, `README.md`, `CHANGELOG.md`, `PROMPT.md`.

## Kickoff task (Iteration 1)
1) Generate the **minimal viable gateway client**:
   - Connect to **Discord Gateway** (WSS), receive **Hello**, send **Identify**, start **Heartbeat**, then cleanly close.
   - Implement in assembly the event loop and heartbeat timer. Use the C shim only for:
     - opening WSS, sending/receiving frames
     - JSON encode/decode calls (suggest a tiny C JSON lib with stable ABI)
     - high-res time & sleep
2) Create:
   - `CMakeLists.txt` for Windows (MSVC + vcpkg or pkg-config), macOS (clang + brew), Linux (gcc).
   - `cshim/` with minimal wrappers and exported C ABI (cdecl on Windows; SysV on Unix).
   - `asm/x64/gateway.asm` (+ include files) with clear labels, macros, and comments.
   - `include/abi.h`, `include/opcodes.h`, `include/structs.h`.
   - `examples/echo/` that logs READY and echoes messages.
3) Write docs:
   - `docs/ADRs/ADR-0001-mvp.md` explaining the shim choice, ABIs, and risks.
   - `README.md` with step-by-step build/run for Windows/macOS/Linux.
   - `CHANGELOG.md` with an “Unreleased” + “Added” for MVP.
4) Add tests:
   - JSON fixture of `Hello` and a parser check.
   - A heartbeat interval test (unit or scripted).
5) Print a **TODO list** for Iteration 2:
   - Reconnect/resume; rate limit backoff; event dispatcher; message send via REST.

## Iteration template (use after every merge)
1) PLAN: Propose the smallest valuable increment.
2) CODE: Generate diffs to implement it; keep Assembly primary.
3) VERIFY: Propose basic tests or a fixture.
4) DOCS: Update ADRs/README/CHANGELOG describing what and why.
5) NEXT: Output the next TODO list with risk notes.

## Quality bar
- Clear comments in assembly (purpose of each label/routine + calling convention).
- No silent changes: every iteration updates ADRs + CHANGELOG.
- Cross-platform CMake targets that CI can build headless.
- Security notes for tokens, TLS, and gateway intents.

## Reference links to use in docs
- Discord Gateway and events: https://discord.com/developers/docs/events/gateway
- Gateway events and opcodes: https://discord.com/developers/docs/events/gateway-events
- Opcodes & status: https://discord.com/developers/docs/topics/opcodes-and-status-codes
