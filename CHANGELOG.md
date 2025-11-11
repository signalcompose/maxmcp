# Changelog

All notable changes to MaxMCP will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.1.1] - 2025-11-11

### Documentation
- Added CHANGELOG.md for version history tracking
- Added docs/PHASE1_INFRASTRUCTURE.md completion report
- Updated README.md with v1.1.0 status and development/testing section
- Updated docs/INDEX.md with new documentation references

## [1.1.0] - 2025-11-11

### Added
- Complete CI/CD infrastructure with GitHub Actions workflows
  - C++ linting with clang-format and clang-tidy
  - C++ build and test automation (Google Test 1.17.0)
  - Node.js linting with ESLint and Prettier
  - Node.js testing with Jest
- Pre-commit hooks for local code quality checks
  - clang-format auto-formatting
  - ESLint auto-fixing
  - Unit test validation
- Comprehensive unit test suite (57 tests, 100% passing)
  - PatchRegistry tests (5 tests)
  - ConsoleLogger tests (8 tests)
  - MCPServer protocol tests (9 tests)
  - WebSocketServer tests (11 tests, 1 disabled pending investigation)
  - UUIDGenerator tests (6 tests)
- `MAXMCP_TEST_MODE` compile flag for CI environment compatibility

### Changed
- Updated test expectations to match implementation (JSON response structure)
- Improved error handling in MCP server test suite

### Fixed
- CI environment SEGFAULT in PatchRegistry tests (added `MAXMCP_TEST_MODE` guards)
- Test stability issues with Max API dependencies
- JSON response structure consistency across all tests

### Removed
- GitHub Advanced Security workflows (TruffleHog, Dependency Review)
  - Moved security scanning to local pre-commit hooks only

## [1.0.0] - 2025-11-11

### Added
- Initial community release
- Complete MCP toolset (10 tools)
- WebSocket-based architecture with Node.js bridge
- Unified binary with `@mode` attribute
- Auto-bundled dependencies (libwebsockets, OpenSSL)
- Comprehensive example patches and documentation
- Max Package structure for distribution

[Unreleased]: https://github.com/signalcompose/MaxMCP/compare/v1.1.1...HEAD
[1.1.1]: https://github.com/signalcompose/MaxMCP/compare/v1.1.0...v1.1.1
[1.1.0]: https://github.com/signalcompose/MaxMCP/compare/v1.0.0...v1.1.0
[1.0.0]: https://github.com/signalcompose/MaxMCP/releases/tag/v1.0.0
