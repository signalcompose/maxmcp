# MaxMCP - Native MCP Server for Max/MSP

Control your Max/MSP patches with Claude Code using natural language.

## Overview

MaxMCP is a native C++ external object for Max/MSP that acts as an MCP (Model Context Protocol) server. It enables Claude Code to control Max/MSP patches through natural language commands, with zero configuration required from users.

## Key Features

- ✅ **Zero configuration**: Just place `[maxmcp]` in your patch
- ✅ **Automatic patch detection**: Auto-generated patch IDs
- ✅ **Natural language control**: "Add a 440Hz oscillator to synth patch"
- ✅ **Multi-patch support**: Control multiple patches simultaneously
- ✅ **Complete MCP toolset**: 10 tools for comprehensive patch control
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
- **WebSocket**: libwebsockets (bundled)
- **TLS**: OpenSSL 3.x (bundled)
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

## Quick Start

### 1. Open Help Patch
In Max/MSP:
- Help → MaxMCP Package → `00-index.maxpat`

Or manually:
```bash
open ~/Documents/Max\ 9/Packages/MaxMCP/examples/00-index.maxpat
```

### 2. Start MCP Agent & Bridge
In `00-index.maxpat`, click the "START" message to launch the agent and bridge.

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

MaxMCP provides 10 tools for comprehensive patch control:

1. **list_active_patches** - List all registered patches
2. **get_console_log** - Retrieve Max Console messages
3. **add_max_object** - Create new Max objects
4. **get_objects_in_patch** - List all objects in a patch
5. **set_object_attribute** - Modify object attributes
6. **connect_max_objects** - Create patchcords
7. **disconnect_max_objects** - Remove patchcords
8. **remove_max_object** - Delete objects
9. **get_patch_info** - Get patch metadata
10. **get_avoid_rect_position** - Find safe object placement positions

## Development Status

**Current Version**: v1.1.0 ✅

✅ **Completed**:
- Phase 1: Core external objects, WebSocket server
- Phase 2: Complete MCP toolset, E2E testing, Max Package integration
- Phase 1 Infrastructure: CI/CD pipeline, comprehensive testing (57 tests), code quality automation

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

## Example Patches

The package includes comprehensive example patches in `examples/`:

- **00-index.maxpat** - Main help/index (also available via right-click → "Open maxmcp Help")
- **01-basic-registration.maxpat** - Basic patch registration
- **02-basic-client.maxpat** - Client mode demonstration
- **03-custom-alias.maxpat** - Custom alias usage
- **04-group-assignment.maxpat** - Group filtering
- **05-06-07-multi-patch-*.maxpat** - Multi-patch scenarios
- **01-claude-code-connection.maxpat** - E2E test scenarios

## Troubleshooting

### Agent won't start
- Check that port 7400 is available: `lsof -i :7400`
- Check Max Console for error messages

### Claude Code can't connect
- Verify bridge is running: Check Max Console for "Bridge launched" message
- Restart Claude Code after `claude mcp add` command
- Verify MCP config: `cat ~/.config/claude-code/mcp.json`

### Objects not appearing
- Ensure patch is unlocked (Cmd+E)
- Check patch has `[maxmcp]` object registered
- Verify patch ID: Use `list_active_patches` tool

## License

MaxMCP is licensed under the **MIT License**. See [LICENSE](LICENSE) for the full license text.

### Third-Party Licenses

MaxMCP uses open-source libraries. See [THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md) for full attribution and license details.

## Development & Testing

MaxMCP uses a comprehensive CI/CD pipeline with automated testing and code quality checks:

- **Testing**: 57 unit tests (Google Test 1.17.0) with 100% pass rate
- **CI/CD**: GitHub Actions workflows for automated testing and linting
- **Code Quality**: Pre-commit hooks with clang-format, ESLint, and automated tests
- **Local Setup**: `pre-commit install` to enable local quality checks

See [docs/PHASE1_INFRASTRUCTURE.md](docs/PHASE1_INFRASTRUCTURE.md) for details on the testing infrastructure.

## Contributing

We welcome contributions from the community! Please see:

- [CONTRIBUTING.md](CONTRIBUTING.md) - How to contribute
- [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) - Community guidelines
- [SECURITY.md](SECURITY.md) - Security policy

All contributions are automatically validated by CI to ensure code quality and test coverage.

## Author

Hiroshi Yamato

## Support

- GitHub Issues: https://github.com/signalcompose/MaxMCP/issues
- Documentation: See `docs/` directory
- Security: See [SECURITY.md](SECURITY.md) for vulnerability reporting

## Inspiration

This project was inspired by [MaxMSP-MCP-Server-multipatch](https://github.com/dropcontrol/MaxMSP-MCP-Server-multipatch), reimagined with a native C++ architecture for improved performance and Max integration.
