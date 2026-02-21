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

### Overall Diagram
```
┌─────────────────────────────────────────────────────────┐
│              Claude Code (MCP Client)                    │
└────────────────────┬────────────────────────────────────┘
                     │ stdio (JSON-RPC)
            ┌────────▼─────────┐
            │  Node.js Bridge  │ ← websocket-mcp-bridge.js
            └────────┬─────────┘
                     │ WebSocket (ws://localhost:7400)
         ┌───────────▼─────────────┐
         │ [maxmcp @mode agent]    │ ← Singleton MCP Server
         └───────────┬─────────────┘
                     │
        ┌────────────┼────────────┐
        │            │            │
   ┌────▼─────┐ ┌───▼─────┐ ┌───▼─────┐
   │[maxmcp   │ │[maxmcp  │ │[maxmcp  │
   │@mode     │ │@mode    │ │@mode    │
   │ patch]   │ │ patch]  │ │ patch]  │
   └──────────┘ └─────────┘ └─────────┘
```

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

All 26 MCP tools are documented in [mcp-tools-reference.md](mcp-tools-reference.md).

| Category | Count | Tools |
|----------|-------|-------|
| Patch Management | 3 | `list_active_patches`, `get_patch_info`, `get_frontmost_patch` |
| Object Operations | 12 | `add_max_object`, `remove_max_object`, `get_objects_in_patch`, `set_object_attribute`, `get_object_attribute`, `get_object_value`, `get_object_io_info`, `get_object_hidden`, `set_object_hidden`, `redraw_object`, `replace_object_text`, `assign_varnames` |
| Connection Operations | 4 | `connect_max_objects`, `disconnect_max_objects`, `get_patchlines`, `set_patchline_midpoints` |
| Patch State | 3 | `get_patch_lock_state`, `set_patch_lock_state`, `get_patch_dirty` |
| Hierarchy | 2 | `get_parent_patcher`, `get_subpatchers` |
| Utilities | 2 | `get_console_log`, `get_avoid_rect_position` |

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

### File Organization
```
src/
├── maxmcp.cpp              # Unified external (@mode agent / @mode patch)
├── maxmcp.h
├── mcp_server.cpp          # MCP protocol handler (JSON-RPC)
├── mcp_server.h
├── websocket_server.cpp    # libwebsockets WebSocket server
├── websocket_server.h
├── tools/
│   ├── patch_tools.cpp     # Patch management (3 tools)
│   ├── object_tools.cpp    # Object operations (12 tools)
│   ├── connection_tools.cpp # Connection operations (4 tools)
│   ├── state_tools.cpp     # Patch state (3 tools)
│   ├── hierarchy_tools.cpp # Hierarchy (2 tools)
│   ├── utility_tools.cpp   # Utilities (2 tools)
│   └── tool_common.cpp     # Shared tool helpers
└── utils/
    ├── patch_helpers.cpp   # Patcher API helpers
    ├── patch_registry.cpp  # Patch registration
    ├── console_logger.cpp  # Console log capture
    └── uuid_generator.cpp  # Patch ID generation

tests/unit/
├── test_mcp_server.cpp
├── test_patch_registry.cpp
├── test_uuid_generator.cpp
└── ...

package/MaxMCP/             # Build output (Max Package)
└── support/bridge/
    └── websocket-mcp-bridge.js
```

### CMakeLists.txt (Key Parts)
```cmake
cmake_minimum_required(VERSION 3.19)
project(MaxMCP)

set(CMAKE_CXX_STANDARD 17)

# Max SDK
set(C74_MAX_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/max-sdk")
include(${C74_MAX_SDK_PATH}/script/max-sdk-base.cmake)

# Dependencies
find_package(nlohmann_json REQUIRED)
find_package(libwebsockets REQUIRED)

# Source files
set(SOURCES
    src/maxmcp.cpp
    src/mcp_server.cpp
    src/websocket_server.cpp
    src/tools/patch_tools.cpp
    src/tools/object_tools.cpp
    src/tools/connection_tools.cpp
    src/tools/state_tools.cpp
    src/tools/hierarchy_tools.cpp
    src/tools/utility_tools.cpp
    src/tools/tool_common.cpp
    src/utils/patch_helpers.cpp
    src/utils/patch_registry.cpp
    src/utils/console_logger.cpp
    src/utils/uuid_generator.cpp
)

add_library(${PROJECT_NAME} MODULE ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE
    nlohmann_json::nlohmann_json
    websockets
)

include(${C74_MAX_SDK_PATH}/script/max-posttarget.cmake)

# Tests (optional)
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
```

---

## Build & Deploy

```bash
# Build (configure + build + install to package/MaxMCP)
./build.sh              # Debug build
./build.sh --test       # Debug build with tests
./build.sh Release      # Release build
./build.sh --clean      # Clean build first

# Deploy to Max 9 Packages
./deploy.sh

# Typical workflow
./build.sh --test && ./deploy.sh
```

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
