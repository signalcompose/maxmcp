# MaxMCP - Native MCP Server for Max/MSP

[![Developed with Claude Code](https://img.shields.io/badge/Developed%20with-Claude%20Code-blueviolet)](https://claude.ai/code)

Control your Max/MSP patches with Claude Code using natural language.

## Overview

MaxMCP is a native C++ external object for Max/MSP that acts as an MCP (Model Context Protocol) server. It enables Claude Code to control Max/MSP patches through natural language commands, with zero configuration required from users.

## Key Features

- ✅ **Zero configuration**: Just place `[maxmcp]` in your patch
- ✅ **Automatic patch detection**: Auto-generated patch IDs
- ✅ **Natural language control**: "Add a 440Hz oscillator to synth patch"
- ✅ **Multi-patch support**: Control multiple patches simultaneously
- ✅ **Complete MCP toolset**: 26 tools for comprehensive patch control
- ✅ **Auto-cleanup**: Lifecycle management on patch close

## Architecture

```
Claude Code (MCP Client)
    ↕ stdio (JSON-RPC)
Node.js Bridge (websocket-mcp-bridge.js)
    ↕ WebSocket
[maxmcp] C++ External Object
    ↕ Max API
Max/MSP Patches
```

**Components**:
- **maxmcp.mxo**: Single unified external with two modes:
  - `@mode agent`: WebSocket server, MCP protocol handler (1 per Max instance)
  - `@mode patch`: Patch registration (1 per controllable patch, default)
- **Bridge**: stdio ↔ WebSocket translator (Node.js, launched automatically)

## Tech Stack

- **Language**: C/C++ (Max SDK 8.6+)
- **Build System**: CMake 3.19+
- **Architecture**: arm64 (Apple Silicon native)
- **MCP Protocol**: stdio-based JSON-RPC
- **JSON Library**: nlohmann/json 3.11.0+
- **WebSocket**: libwebsockets (installed via Homebrew, bundled into .mxo at build time)
- **TLS**: OpenSSL 3.x (installed via Homebrew, bundled into .mxo at build time)
- **Code Signing**: Ad-hoc signature (auto-applied)
- **Distribution**: Max Package

## Installation

### Option 1: Max Package Manager (Coming Soon)
1. Open Max/MSP
2. File → Show Package Manager
3. Search "MaxMCP"
4. Click Install

### Option 2: Manual Install
1. Download latest release
2. Extract to `~/Documents/Max 9/Packages/`
3. Restart Max

### Option 3: Build from Source (Development Setup)
Use this path if you're cloning the repository and want to build the external yourself.

1. **Clone the repo**
   ```bash
   git clone https://github.com/signalcompose/MaxMCP.git
   cd MaxMCP
   ```
2. **Install prerequisites**
   ```bash
   brew install cmake nlohmann-json libwebsockets openssl
   ```
   - Requires macOS 13+, Xcode Command Line Tools, Max 9.1+, Node.js 18+ with npm.
3. **Fetch the Max SDK with submodules**
   ```bash
   git clone https://github.com/Cycling74/max-sdk.git --recursive max-sdk
   ```
   The `--recursive` flag is critical; without it `max-pretarget.cmake` is missing.
4. **Install bridge dependencies**
   ```bash
   cd package/MaxMCP/support/bridge
   npm install
   cd ../../../..
   ```
   This installs the `ws` dependency used by `websocket-mcp-bridge.js`.
5. **Build the external**
   ```bash
   ./build.sh --clean Release   # optional but recommended for first build
   ./build.sh Release
   ```
   The script configures CMake, builds the external, and installs `maxmcp.mxo`
   into `package/MaxMCP/externals/`. Confirm the bundle exists:
   ```bash
   ls package/MaxMCP/externals/maxmcp.mxo/Contents/MacOS/maxmcp
   ```
6. **Deploy to Max 9 Packages**
   ```bash
   ./deploy.sh
   ```
   This copies `package/MaxMCP` to `~/Documents/Max 9/Packages/MaxMCP`.
7. **(Optional) Manual CMake invocation**
   ```bash
   cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
   cmake --build build
   cmake --install build --prefix package/MaxMCP
   ```
8. **Verify bridge tooling**
   ```bash
   cd package/MaxMCP/support/bridge
   npm test   # runs Jest suite against websocket-mcp-bridge.js
   ```

## Quick Start

### 1. Open Help Patch
In Max/MSP:
- Help → MaxMCP Package → `00-index.maxpat`

Or manually:
```bash
open ~/Documents/Max\ 9/Packages/MaxMCP/examples/00-index.maxpat
```

### 2. Start MCP Agent & Bridge
**Option A (recommended):** In `00-index.maxpat`, click the "START" message to launch the agent and bridge automatically.

**Option B (manual patch):**
1. Unlock a new patcher (Cmd+E) and add:
   ```
   [maxmcp @mode agent @port 7400]
   ```
2. Add a message box with `START` and connect it to the agent inlet.
3. Click the `START` message. The Max Console should log:
   ```
   WebSocket server started on port 7400
   maxmcp: maxmcp (agent mode) started on port 7400
   ```
4. Start the Node bridge in a terminal:
   ```bash
   node ~/Documents/Max\ 9/Packages/MaxMCP/support/bridge/websocket-mcp-bridge.js ws://localhost:7400
   ```
   Run `npm install` inside `package/MaxMCP/support/bridge/` first if you have not already.

### 3. Configure Claude Code
Run this command in your terminal:
```bash
claude mcp add maxmcp node ~/Documents/Max\ 9/Packages/MaxMCP/support/bridge/websocket-mcp-bridge.js ws://localhost:7400
```

Restart Claude Code to apply changes.

### 4. Create Controllable Patch
In your Max patch, add:
```
[maxmcp @alias my-synth @group synth]
```

Attributes:
- `@mode patch` - Client mode (default, can be omitted)
- `@alias my-synth` - Custom patch ID (optional, auto-generated if omitted)
- `@group synth` - Group name for filtering (optional)

### 5. Control with Natural Language
In Claude Code, say:
- "List all active Max patches"
- "Add a 440Hz oscillator to my-synth patch"
- "Connect the oscillator to dac~"
- "Show me the Max Console log"

## Available MCP Tools

MaxMCP provides 26 tools across 6 categories:

| Category | Count | Tools |
|----------|-------|-------|
| Patch Management | 3 | `list_active_patches`, `get_patch_info`, `get_frontmost_patch` |
| Object Operations | 12 | `add_max_object`, `remove_max_object`, `get_objects_in_patch`, `set_object_attribute`, `get_object_attribute`, `get_object_value`, `get_object_io_info`, `get_object_hidden`, `set_object_hidden`, `redraw_object`, `replace_object_text`, `assign_varnames` |
| Connection Operations | 4 | `connect_max_objects`, `disconnect_max_objects`, `get_patchlines`, `set_patchline_midpoints` |
| Patch State | 3 | `get_patch_lock_state`, `set_patch_lock_state`, `get_patch_dirty` |
| Hierarchy | 2 | `get_parent_patcher`, `get_subpatchers` |
| Utilities | 2 | `get_console_log`, `get_avoid_rect_position` |

See [docs/mcp-tools-reference.md](docs/mcp-tools-reference.md) for full parameter and response documentation.

## Development Status

**Current Version**: v1.1.0 ✅

✅ **Completed**:
- Phase 1: Core external objects, WebSocket server
- Phase 2: Complete MCP toolset, E2E testing, Max Package integration
- Phase 1 Infrastructure: CI/CD pipeline, comprehensive testing (117 tests), code quality automation

🔄 **Next**:
- Phase 3: Max Package Manager submission
- Phase 4: Cross-platform builds (Windows/Intel support)

See [CHANGELOG.md](CHANGELOG.md) for version history and `docs/` for detailed specifications and development roadmap.

## Documentation

- [CHANGELOG](CHANGELOG.md) - Version history and release notes
- [Documentation Index](docs/INDEX.md) - Complete documentation overview
- [Quick Start Guide](docs/quick-start.md) - Detailed getting started
- [Development Guide](docs/development-guide.md) - Contributing guide
- [Architecture](docs/architecture.md) - System design
- [Specifications](docs/specifications.md) - Technical specs

## Claude Code Plugin

MaxMCP provides a Claude Code plugin with four skills for Max/MSP development.

### Installation

```bash
/plugin marketplace add signalcompose/maxmcp
/plugin install maxmcp@maxmcp
```

### Available Skills

#### patch-guidelines

Guidelines for creating well-organized Max patches:

```bash
/maxmcp:patch-guidelines
```

Provides:
- Layout rules for object positioning
- Varname naming conventions
- JavaScript (v8/v8ui) best practices
- MCP tools quick reference

#### max-techniques

Max/MSP implementation techniques and best practices:

```bash
/maxmcp:max-techniques
```

Provides:
- poly~ & bpatcher architecture patterns
- pattr/pattrstorage parameter management
- Constant parameter safety, sampling rate handling

#### m4l-techniques

Max for Live development techniques and best practices:

```bash
/maxmcp:m4l-techniques
```

Provides:
- Live Object Model (path → id → live.object → live.observer)
- Device namespaces (`---` vs `#0`) and pattr persistence
- Controller mapping, dBFS reference, Push2 automapping

#### max-resources

Access Max.app built-in documentation and examples:

```bash
/maxmcp:max-resources
```

Provides:
- Object reference pages (inlets, outlets, methods, attributes)
- Example patches from Max.app
- Code snippets
- Full-text search of Max documentation

## Example Patches

The package includes comprehensive example patches in `examples/`:

- **00-index.maxpat** - Main help/index (also available via right-click → "Open maxmcp Help")
- **01-basic-registration.maxpat** - Basic patch registration
- **01-claude-code-connection.maxpat** - Claude Code E2E connection test
- **02-custom-alias.maxpat** - Custom alias usage
- **03-group-assignment.maxpat** - Group filtering
- **04-05-06-multi-patch-*.maxpat** - Multi-patch scenarios (synth1, synth2, fx1)
- **07-mcp-tools-test.maxpat** - MCP tools testing

## Troubleshooting

### Agent won't start
- Check that port 7400 is available: `lsof -i :7400`
- If the bridge logs `ECONNREFUSED`, start `[maxmcp @mode agent @port 7400]` and click `START`
- Check Max Console for error messages

### Claude Code can't connect
- Verify bridge is running: Check Max Console for "Bridge launched" message
- Restart Claude Code after `claude mcp add` command
- Check MCP configuration with `claude mcp list`
- Run the bridge with debugging to capture WebSocket errors:
  `DEBUG=1 node ~/Documents/Max\ 9/Packages/MaxMCP/support/bridge/websocket-mcp-bridge.js ws://localhost:7400`

### Objects not appearing
- Ensure patch is unlocked (Cmd+E)
- Check patch has `[maxmcp]` object registered
- Verify patch ID: Use `list_active_patches` tool

## License

MaxMCP is licensed under the **Signal compose Fair Trade License v1.0**. See [LICENSE](LICENSE) for the full license text.

### Third-Party Licenses

MaxMCP uses open-source libraries. See [THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md) for full attribution and license details.

## Development & Testing

MaxMCP uses a comprehensive CI/CD pipeline with automated testing and code quality checks:

- **Testing**: 117 unit tests (Google Test 1.17.0) with 100% pass rate
- **CI/CD**: GitHub Actions workflows for automated testing and linting
- **Code Quality**: Pre-commit hooks with clang-format, ESLint, and automated tests
- **Local Setup**: `npm install` to enable pre-commit hooks via Husky

See [docs/development-guide.md](docs/development-guide.md) for details on the testing infrastructure.

## Contributing

We welcome contributions from the community! Please see:

- [CONTRIBUTING.md](CONTRIBUTING.md) - How to contribute
- [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) - Community guidelines
- [SECURITY.md](SECURITY.md) - Security policy

This project uses [Claude Code](https://claude.ai/code) for development. See [CLAUDE.md](CLAUDE.md) for AI assistant guidelines and project conventions.

All contributions are automatically validated by CI to ensure code quality and test coverage.

## Author

Hiroshi Yamato

## Support

- GitHub Issues: https://github.com/signalcompose/MaxMCP/issues
- Documentation: See `docs/` directory
- Security: See [SECURITY.md](SECURITY.md) for vulnerability reporting

## Inspiration

This project was inspired by [MaxMSP-MCP-Server-multipatch](https://github.com/dropcontrol/MaxMSP-MCP-Server-multipatch), reimagined with a native C++ architecture for improved performance and Max integration.
