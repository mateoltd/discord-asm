# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- MVP Discord Gateway client with Assembly core and C shim architecture
- Cross-platform support for Windows x64, Linux x64, and macOS x64
- Assembly gateway implementation with Hello/Identify/Heartbeat protocol support
- C shim for WebSocket operations using libwebsockets + OpenSSL
- C shim for JSON parsing/generation of Discord Gateway messages
- C shim for high-resolution timing and sleep functions
- Stable cross-platform ABI between Assembly and C components
- CMake build system with automatic dependency detection
- Echo example bot demonstrating basic Gateway connection
- Unit tests for JSON parsing and heartbeat timing functionality
- Test fixtures for Discord Gateway message formats
- ADR-0001 documenting MVP architecture decisions
- Comprehensive README with platform-specific build instructions
- PROMPT.md control specification for development loop
- Ralph loop infrastructure for continuous development

### Dependencies
- libwebsockets for WebSocket client implementation
- OpenSSL for TLS/SSL support
- NASM for Assembly compilation
- CMake 3.20+ for cross-platform builds

### Security
- Bot tokens must be provided via environment variables only
- No hardcoded credentials in source code
- TLS certificate validation enabled for Gateway connections
- Memory safety practices in C shim (bounds checking, null validation)

## [0.1.0] - 2024-10-06

### Added
- Initial project structure and documentation
- Repository scaffolding with directory layout
- License and contributing guidelines

[Unreleased]: https://github.com/mateoltd/discord-asm/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/mateoltd/discord-asm/releases/tag/v0.1.0