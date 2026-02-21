# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

MaxMCP is a native C++ external object for Max/MSP that implements an MCP (Model Context Protocol) server. It enables Claude Code to control Max/MSP patches through natural language commands via a unified external with two modes: `@mode agent` (WebSocket server, MCP handler) and `@mode patch` (client registration).

**Architecture**: Claude Code ↔ stdio ↔ Node.js Bridge ↔ WebSocket ↔ maxmcp.mxo ↔ Max/MSP Patches

## Build & Deploy

```bash
# Build (configure + build + install to package/MaxMCP)
./build.sh              # Debug build
./build.sh --test       # Debug build with tests
./build.sh Release      # Release build
./build.sh --clean      # Clean build first

# Deploy to Max 9 Packages (removes old package automatically)
./deploy.sh

# Typical workflow
./build.sh --test && ./deploy.sh
```

### Manual commands (for reference)

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build build
cd build && ctest --output-on-failure && cd ..
cmake --install build --prefix package/MaxMCP
```

**Test framework**: Google Test 1.17.0
**Test files**: `tests/unit/test_*.cpp`
**Test mode macro**: `MAXMCP_TEST_MODE` (enables compilation without Max SDK)

## Code Quality

**C++ formatting**: clang-format (run before commit)
```bash
find src -name "*.cpp" -o -name "*.h" | xargs clang-format -i
```

**Node.js (bridge)**: ESLint + Prettier
```bash
cd package/MaxMCP/support/bridge
npm run lint
npm run format:check
```

## Architecture Overview

### Key Components

- **`src/maxmcp.cpp`**: Unified external object supporting both agent and patch modes via `@mode` attribute
- **`src/mcp_server.cpp`**: MCP protocol handler (JSON-RPC), implements all MCP tools
- **`src/websocket_server.cpp`**: libwebsockets-based WebSocket server for bridge communication
- **`src/utils/`**: Shared utilities (UUID generator, console logger, patch registry, patch helpers)

### MCP Tools (26 total)

Full tool reference with parameters and response formats: [docs/mcp-tools-reference.md](docs/mcp-tools-reference.md)

| Category | Count | Tools |
|----------|-------|-------|
| Patch Management | 3 | `list_active_patches`, `get_patch_info`, `get_frontmost_patch` |
| Object Operations | 12 | `add_max_object`, `remove_max_object`, `get_objects_in_patch`, `set_object_attribute`, `get_object_attribute`, `get_object_value`, `get_object_io_info`, `get_object_hidden`, `set_object_hidden`, `redraw_object`, `replace_object_text`, `assign_varnames` |
| Connection Operations | 4 | `connect_max_objects`, `disconnect_max_objects`, `get_patchlines`, `set_patchline_midpoints` |
| Patch State | 3 | `get_patch_lock_state`, `set_patch_lock_state`, `get_patch_dirty` |
| Hierarchy | 2 | `get_parent_patcher`, `get_subpatchers` |
| Utilities | 2 | `get_console_log`, `get_avoid_rect_position` |

### Threading Model

All Max API calls must run on the main thread. Use `defer()` for operations triggered from WebSocket callbacks:

```
WebSocket Thread → defer() → Max Main Thread → Execute Max API
```

## Dependencies

- **C++17** (required)
- **Max SDK 8.6+** (clone to `max-sdk/` directory)
- **nlohmann/json 3.11.0+** (`brew install nlohmann-json`)
- **libwebsockets** (`brew install libwebsockets`)
- **Google Test** (for tests only, `brew install googletest`)

## Git Workflow

**Branch strategy**: GitHub Flow

- `main` - Production (default branch, protected)
- `feature/XX-description` - Feature branches (XX = issue number)
- `docs/description` - Documentation updates
- `bugfix/description` - Bug fixes

All changes merged to `main` via Pull Request with required CI checks.

**Commit format**: Conventional Commits with English titles

```
feat(tools): add disconnect_max_objects MCP tool

Implement MCP tool for disconnecting patchcords.
- Parse source and destination object varnames
- Defer to main thread for safety

Closes #99
```

## Release Management

**Tool**: [Release Please](https://github.com/googleapis/release-please) (automated)

Conventional Commits automatically determine semantic versioning:
- `feat:` → MINOR (new feature: 1.0.0 → 1.1.0)
- `fix:` → PATCH (bug fix: 1.0.0 → 1.0.1)
- `feat!:` or `BREAKING CHANGE:` → MAJOR (breaking: 1.0.0 → 2.0.0)

### Release Workflow

When PRs are merged to `main`, Release Please automatically:
1. Creates/updates a release PR with CHANGELOG entries
2. On release PR merge: creates Git tag and GitHub Release

### Release Rules (MUST follow)

| ✅ DO | ❌ DON'T |
|-------|----------|
| Merge Release Please PR to create releases | Create tags manually (`git tag`) |
| Let Release Please manage CHANGELOG | Edit CHANGELOG.md manually |
| Use Conventional Commits for versioning | Manually bump version numbers |

**Release Process**:
1. Merge feature/fix PRs to `main` with Conventional Commits
2. Release Please automatically creates/updates release PR
3. Review the release PR (check CHANGELOG, version bump)
4. Merge the release PR → Tag and GitHub Release created automatically

## Coding Conventions

- **Classes**: PascalCase (`MCPServer`)
- **Functions**: snake_case (`generate_patch_id`)
- **Variables**: snake_case (`patch_id`)
- **Constants**: UPPER_SNAKE_CASE (`MAX_BUFFER_SIZE`)
- **Private members**: trailing underscore (`name_`)
- **Indentation**: 4 spaces
- **Max line length**: 100 characters

## Documentation

Documentation is the single source of truth (DDD - Documentation Driven Development):

- `docs/INDEX.md` - Documentation index
- `docs/specifications.md` - Technical specifications
- `docs/architecture.md` - System design
- `docs/development-guide.md` - Development practices

Always read and update documentation before/after code changes.

## Skill Loading Rules (MCP Patch Operations)

When performing Max/MSP patch operations via MCP tools, **always load the relevant skills before starting work**. This ensures consistent quality and adherence to project conventions.

| Skill | Condition | Command |
|-------|-----------|---------|
| **patch-guidelines** | Always load when operating on Max patches | `/maxmcp:patch-guidelines` |
| **max-techniques** | Load when implementing Max/MSP features (poly~, pattr, signal processing, etc.) | `/maxmcp:max-techniques` |
| **m4l-techniques** | Load only when working on Max for Live devices | `/maxmcp:m4l-techniques` |
| **max-resources** | Load as needed when looking up object references or examples | `/maxmcp:max-resources` |

**Priority**: patch-guidelines is mandatory for all patch operations. Other skills are loaded based on the task context.

## Claude Code Plugin Marketplace

MaxMCP provides a Claude Code plugin marketplace for patch creation guidelines.

### Plugin Installation

```bash
# Add marketplace
/plugin marketplace add signalcompose/maxmcp

# Install plugin
/plugin install maxmcp@maxmcp
```

### Available Skills

#### patch-guidelines

Guidelines for creating well-organized Max patches.

```bash
/maxmcp:patch-guidelines
```

Provides:
- Layout rules for object positioning
- Varname naming conventions
- JavaScript (v8/v8ui) best practices
- MCP tools quick reference

#### max-techniques

Max/MSP implementation techniques and best practices.

```bash
/maxmcp:max-techniques
```

Provides:
- poly~ & bpatcher architecture patterns
- pattr/pattrstorage parameter management
- Constant parameter safety, sampling rate handling

#### m4l-techniques

Max for Live development techniques and best practices.

```bash
/maxmcp:m4l-techniques
```

Provides:
- Live Object Model (path → id → live.object → live.observer)
- Device namespaces (`---` vs `#0`) and pattr persistence
- Controller mapping, dBFS reference, Push2 automapping

#### max-resources

Access Max/MSP built-in documentation and examples.

```bash
/maxmcp:max-resources
```

Provides:
- Object reference pages (inlets, outlets, methods, attributes)
- Example patches from Max.app
- Code snippets
- Full-text search of Max documentation

**Agentic search approach**: This skill uses direct filesystem exploration instead of pre-built indexes. No setup required - information is always current.

**Workflow example**:
1. User asks "How do I use cycle~?"
2. AI searches Max.app refpages directly: `find ... -name "cycle~.maxref.xml"`
3. Reads XML and extracts reference information (digest, inlets, outlets, methods)
4. Can also find related examples and snippets via filesystem search

### Plugin Structure

```
plugins/
└── maxmcp/
    ├── .claude-plugin/plugin.json
    ├── skills/
    │   ├── patch-guidelines/
    │   │   ├── SKILL.md
    │   │   └── reference/
    │   ├── max-techniques/
    │   │   ├── SKILL.md
    │   │   └── reference/        # poly~, bpatcher, pattr, tips
    │   ├── m4l-techniques/
    │   │   ├── SKILL.md
    │   │   └── reference/        # LOM, namespaces, M4L tips
    │   └── max-resources/
    │       ├── SKILL.md
    │       ├── scripts/        # Search and retrieval scripts
    │       ├── references/     # Format documentation
    │       └── examples/       # Usage examples
    └── README.md
```
