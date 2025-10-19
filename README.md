# MaxMCP v2.0 - Native MCP Server for Max/MSP

Control your Max/MSP patches with Claude Code using natural language.

## Overview

MaxMCP is a native C++ external object for Max/MSP that acts as an MCP (Model Context Protocol) server. It enables Claude Code to control Max/MSP patches through natural language commands, with zero configuration required from users.

## Key Features

- ✅ **Zero configuration**: Just place `[maxmcp]` in your patch
- ✅ **Automatic patch detection**: Auto-generated patch IDs
- ✅ **Natural language control**: "Add a 440Hz oscillator to synth patch"
- ✅ **Multi-patch support**: Control multiple patches simultaneously
- ✅ **Auto-cleanup**: Lifecycle management on patch close

## Architecture

```
Claude Code (MCP Client)
    ↕ stdio (JSON-RPC)
[maxmcp] C++ External Object
    ↕ Max API
Max/MSP Patches
```

## Tech Stack

- **Language**: C/C++ (Max SDK 8.6+)
- **Build System**: CMake
- **MCP Protocol**: stdio-based JSON-RPC
- **JSON Library**: nlohmann/json
- **Distribution**: Max Package

## What We're Replacing

This v2.0 implementation replaces:
- ❌ Python MCP Server
- ❌ Node.js Socket.IO Server
- ❌ 6 JavaScript files (max_mcp_node.js, mcp-router.js, etc.)
- ✅ **1 C++ external object** (99% reduction)

## Installation

### Option 1: Max Package Manager (Recommended)
1. Open Max/MSP
2. File → Show Package Manager
3. Search "MaxMCP"
4. Click Install

### Option 2: Manual Install
1. Download latest release
2. Extract to `~/Documents/Max 9/Packages/`
3. Restart Max

## Quick Start

1. Create new patch in Max
2. Add object: `[maxmcp]`
3. Open Claude Code
4. Say: "Add a 440Hz oscillator to my patch"

That's it! No configuration needed.

## Development Status

**Current Phase**: Bootstrap (Setting up project structure)

See `docs/` for detailed specifications and development roadmap.

## Documentation

- [Complete Design Specification](docs/MAXMCP_V2_DESIGN.md)
- [Quick Start Guide](docs/README.md)
- [Development Guide](docs/development-guide.md)
- [Architecture](docs/architecture.md)

## License

MIT License

## Author

Hiroshi Yamato

## Support

- GitHub Issues: https://github.com/dropcontrol/MaxMCP/issues
- Documentation: Coming soon
