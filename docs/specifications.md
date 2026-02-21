# MaxMCP - Complete Design Specification

**Last Updated**: 2026-02-22
**Status**: 26 MCP Tools Implemented

> **Comprehensive specification for the project.**
> A new Claude Code instance can read this document and immediately understand the system.

---

## Project Overview

### Mission
Develop a native MCP server external object for Max/MSP, enabling Claude Code to control Max/MSP patches using natural language.

### Goals
1. **One-click installation**: Via Max Package Manager
2. **Zero configuration**: Works immediately after placing `[maxmcp @mode patch]`
3. **Full automation**: Auto-generated patch IDs, lifecycle management
4. **Natural language**: "Add oscillator to synth patch" just works

### Tech Stack
- **C/C++17** (Max SDK 8.6+)
- **WebSocket** (libwebsockets)
- **Node.js Bridge** (stdio-to-WebSocket translation)
- **CMake** (Cross-platform builds)
- **JSON** (nlohmann/json)
- **Testing** (Google Test)

---

## System Architecture

For detailed architecture diagrams and component interactions, see [architecture.md](architecture.md).

### Architecture Approach

**Unified External Design**: Single `maxmcp.mxo` with `@mode` attribute selecting role.

- `@mode agent` — WebSocket server, MCP protocol handler, patch registry, console logger
- `@mode patch` — Client registration, auto-generated patch ID, lifecycle monitoring

### Communication Protocol

```
Claude Code (stdio MCP)
    ↓ stdin/stdout
websocket-mcp-bridge.js (Node.js)
    ↓ WebSocket (ws://localhost:7400)
maxmcp.mxo @mode agent (C++ / libwebsockets)
    ↓ Max API
Max/MSP Patches
```

### WebSocket Server

**Library**: libwebsockets
**Protocol**: WebSocket over TCP
**Default Port**: 7400
**Message Format**: JSON-RPC 2.0 (text frames)

**Connection Flow**:
1. Bridge connects to `ws://localhost:7400`
2. Bidirectional JSON-RPC message exchange
3. Server maintains connection until client disconnects

**Thread Safety**:
1. libwebsockets event loop runs in background thread
2. `defer()` ensures processing on Max main thread
3. All Max API calls occur on main thread only

### WebSocket MCP Bridge (websocket-mcp-bridge.js)

**Purpose**: Translate stdio MCP (Claude Code) to WebSocket (Max)

**Location**: `package/MaxMCP/support/bridge/websocket-mcp-bridge.js`

**Usage**:
```bash
node websocket-mcp-bridge.js ws://localhost:7400
```

**MCP Server Configuration** (Claude Code):
```bash
claude mcp add maxmcp node \
  ~/Documents/Max\ 9/Packages/MaxMCP/support/bridge/websocket-mcp-bridge.js \
  ws://localhost:7400
```

---

## Key Features

### 1. Auto-Generated Patch IDs

```
patch_id = "{patchname}_{uuid_short}"
Examples: "synth_a7f2b3c9", "myeffect_b3e1d5a2", "master_c9d4e6f8"
```

#### User Experience
```
Before: [js mcp_client.js my_synth_patch_v2]  ← Manual input
After:  [maxmcp @mode patch]                   ← No arguments!
```

---

### 2. Lifecycle Management

#### State Transitions
```
[New] → loadbang → [Registered] → close → [Unregistered] → [Destroyed]
```

- **Auto-registration**: On `loadbang`, patch registers with agent's patch registry
- **Auto-unregistration**: On patcher close event, patch unregisters automatically
- **No manual cleanup needed**

---

### 3. MCP Tools (26 total)

26 MCP tools across 6 categories: Patch Management (3), Object Operations (12), Connection Operations (4), Patch State (3), Hierarchy (2), Utilities (2).

Complete tool reference with parameters and response formats: [mcp-tools-reference.md](mcp-tools-reference.md).

---

### 4. Fuzzy Matching (Claude Code Side)

Claude Code handles natural language resolution on the client side:
1. `list_active_patches()` → get all registered patches
2. Match user input against `display_name` fields
3. Auto-select if unambiguous, ask user if ambiguous

---

## Max Package Structure

### Directory Layout
```
MaxMCP/
├── externals/
│   └── maxmcp.mxo              # macOS (arm64)
├── help/
│   └── maxmcp.maxhelp          # Help patch
├── docs/
│   └── refpages/
│       └── maxmcp.maxref.xml   # Max Reference integration
├── support/
│   └── bridge/
│       ├── websocket-mcp-bridge.js  # stdio-to-WebSocket bridge
│       ├── package.json
│       └── node_modules/
├── package-info.json
├── README.md
└── LICENSE
```

---

## Source Code Structure

For the detailed file organization and CMakeLists.txt structure, see [development-guide.md](development-guide.md) §3 (Build System) and §4.1 (File Organization).

---

## Build & Deploy

```bash
./build.sh --test && ./deploy.sh
```

For all build options and deployment details, see [development-guide.md](development-guide.md) §3.2 and §9.

---

## Testing Strategy

### Unit Tests (Google Test)
```bash
./build.sh --test
# or
cd build && ctest --output-on-failure
```

**Test mode macro**: `MAXMCP_TEST_MODE` enables compilation without Max SDK.

### E2E Tests
Manual testing with Claude Code — see [manual-test-new-tools.md](manual-test-new-tools.md).

---

## Development Phases

### Phase 1: MVP ✅ Complete
- [x] Max SDK setup and CMake build system
- [x] Unified external (`@mode agent` / `@mode patch`)
- [x] WebSocket communication
- [x] 3 core MCP tools

### Phase 2: Core Features ✅ Complete
- [x] All 10 MCP tools
- [x] Auto-generated patch IDs
- [x] Lifecycle management
- [x] stdio-to-WebSocket bridge

### Post-Phase 2: Tool Expansion ✅ Complete
- [x] 26 MCP tools across 6 categories
- [x] Claude Code plugin with 4 skills
- [x] Build/deploy scripts

### Phase 3: Package Distribution (Planned)
- [ ] Max Package Manager submission
- [ ] GitHub releases

### Phase 4: Cross-Platform (Planned)
- [ ] Windows build (x64)
- [ ] Intel Mac build (x86_64)

---

## Success Metrics

1. **Installation time**: < 30 seconds (via Package Manager)
2. **Startup time**: < 3 seconds (patch open to MCP ready)
3. **Memory usage**: < 10MB per patch
4. **Response time**: < 100ms per operation
5. **Stability**: 24-hour continuous operation without crashes

---

## References

- [Max SDK Documentation](https://github.com/Cycling74/max-sdk)
- [MCP Specification](https://spec.modelcontextprotocol.io/)
- [MCP Tools Reference](mcp-tools-reference.md)
- [Architecture](architecture.md)
- [Development Guide](development-guide.md)

---

**This specification follows DDD (Documentation Driven Development) and serves as the single source of truth for MaxMCP.**
