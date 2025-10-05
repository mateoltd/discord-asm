# ADR-0001: MVP Architecture and C Shim Design

**Date:** 2024-10-06  
**Status:** Accepted  
**Context:** Initial architecture decisions for discord-asm MVP

## Decision

We will implement the Discord Gateway client using a **hybrid architecture** with Assembly as the primary language and a minimal C shim for platform-specific operations.

### Architecture Components

1. **Assembly Core** (`asm/x64/gateway.asm`)
   - Main event loop and state management
   - Gateway protocol logic (Hello, Identify, Heartbeat)
   - Calling convention abstraction for cross-platform support
   - Direct interface to bot authors

2. **C Shim** (`cshim/`)
   - WebSocket operations (connect, send, receive, close)
   - JSON parsing and generation for Gateway messages
   - High-resolution timing and sleep functions
   - Platform-specific networking (TLS, sockets)

3. **Stable ABI** (`include/abi.h`)
   - C calling convention interface between Assembly and C
   - Structure definitions for cross-platform compatibility
   - Error code definitions

### Key Design Principles

- **Assembly-first API**: Bot authors interact primarily with Assembly functions
- **Minimal C surface**: C shim only handles what Assembly cannot do efficiently
- **Cross-platform ABI**: Single Assembly codebase works on Windows/Linux/macOS
- **Vendor library isolation**: libwebsockets and OpenSSL wrapped in stable interface

## Rationale

### Why Assembly + C Shim vs Pure Assembly?

**Pros of Hybrid Approach:**
- Assembly handles performance-critical event loop and state logic
- C shim provides pragmatic access to mature crypto/networking libraries
- Faster development than implementing TLS/WebSocket from scratch in Assembly
- Easier to audit security-critical components (smaller C surface)
- Cross-platform networking "just works" via established libraries

**Cons Considered:**
- Additional ABI boundary between Assembly and C
- Dependency on external libraries (libwebsockets, OpenSSL)
- Slightly more complex build system

**Alternative Rejected: Pure Assembly**
- Would require implementing WebSocket framing, TLS handshake, and JSON parsing
- Significantly higher development time and security risk
- Platform-specific socket/crypto code would be complex to maintain

### Why libwebsockets vs Custom WebSocket Implementation?

- **Mature**: Battle-tested in production systems
- **Cross-platform**: Handles Windows/Unix socket differences
- **TLS integration**: Works seamlessly with OpenSSL/mbedTLS
- **Small footprint**: Can be statically linked
- **Stable API**: Unlikely to break ABI compatibility

### Calling Convention Strategy

The C shim uses standard platform calling conventions:
- **Windows x64**: RCX, RDX, R8, R9 + shadow space
- **SysV AMD64** (Linux/macOS): RDI, RSI, RDX, RCX, R8, R9

Assembly code adapts to these conventions using conditional compilation (`%ifdef WINDOWS`).

## Consequences

### Positive
- Rapid MVP development using proven libraries
- Assembly maintains control over performance-critical paths
- Clear security audit boundary (C shim is small and focused)
- Cross-platform support without Assembly complexity

### Negative  
- External dependencies (libwebsockets, OpenSSL) must be managed
- Additional build complexity for library detection/linking
- C shim adds small performance overhead (acceptable for I/O operations)

### Risks and Mitigations

**Risk**: External library vulnerabilities  
**Mitigation**: Pin library versions, document update process in ADRs

**Risk**: ABI breakage between Assembly and C  
**Mitigation**: Comprehensive test suite, strict versioning of ABI structures

**Risk**: Platform-specific calling convention bugs  
**Mitigation**: Platform-specific test matrix, careful conditional compilation

## Implementation Notes

- C shim functions must be thread-safe (though MVP is single-threaded)
- Memory management: C allocates, Assembly uses, C frees
- Error handling: C functions return `discord_result_t` codes
- All strings are null-terminated C strings for simplicity

## Next ADRs

- ADR-0002: Detailed calling convention specifications
- ADR-0003: Error handling and logging strategy  
- ADR-0004: WebSocket message buffering and flow control