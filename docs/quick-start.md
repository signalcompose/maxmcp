# MaxMCP - Quick Start Guide

> **For Claude Code**: Start by reading documentation in `docs/` directory for complete specifications.

---

## Project Goals

Create a **native C++ external object** for Max/MSP that:
- Acts as an MCP server (stdio-based)
- Enables Claude Code to control Max/MSP patches
- Requires zero configuration from users
- Distributes as a Max Package (1-click install)

---

## Architecture Summary

### What We're Building
```
Claude Code (MCP Client)
    ↕ stdio (JSON-RPC)
[maxmcp] C++ External Object
    ↕ Max API
Max/MSP Patches
```

### Architecture Approach
- ✅ **Single C++ external object**
- ✅ **stdio-based MCP protocol**
- ✅ **No external dependencies at runtime**

---

## Key Features

### 1. Auto-Generated Patch IDs
```
User places:  [maxmcp]
System creates: patch_id = "synth_a7f2"
Claude sees: "synth patch" → automatically resolves to "synth_a7f2"
```

### 2. Automatic Lifecycle
- **Open patch** → Auto-register
- **Close patch** → Auto-unregister
- **No manual cleanup needed**

### 3. Natural Language Control
```
User: "Add a 440Hz oscillator to synth patch"
Claude Code:
  1. list_active_patches() → ["synth_a7f2", "fx_b3e1"]
  2. Fuzzy match "synth" → "synth_a7f2"
  3. add_max_object(patch_id="synth_a7f2", obj_type="cycle~", args=[440])
```

---

## Tech Stack

### Confirmed Choices
- **Language**: C/C++ (Max SDK 8.6+)
- **Build System**: CMake
- **MCP Protocol**: stdio-based JSON-RPC
- **JSON Library**: nlohmann/json
- **Distribution**: Max Package

### Rationale
- **C++**: Most stable, direct Max API access, no external dependencies
- **stdio**: Simplest MCP protocol, no network overhead
- **Max Package**: Best user experience (Package Manager install)

---

## Project Structure

```
MaxMCP/
├── src/                    # C++ source code
│   ├── maxmcp.cpp         # Main external object
│   ├── mcp_server.cpp     # stdio MCP server
│   └── tools/             # MCP tool implementations
├── tests/                  # Unit tests
├── examples/               # Example Max patches
├── externals/             # Compiled .mxo/.mxe64 files
└── package-info.json      # Max Package metadata
```

---

## Development Phases

### Phase 1: MVP (Week 1-2)
**Goal**: Basic functioning external object

- [ ] Max SDK setup
- [ ] Minimal `[maxmcp]` object that posts to Max console
- [ ] stdio communication (read from stdin, write to stdout)
- [ ] Implement 2 tools:
  - `list_active_patches()`
  - `add_max_object()`

**Success**: Claude Code can add one object to a patch

### Phase 2: Core (Week 3-4)
**Goal**: All features working

- [ ] Auto-generated patch IDs
- [ ] Lifecycle monitoring (close → unregister)
- [ ] All MCP tools (connect, disconnect, get_objects, etc.)
- [ ] docs.json integration

**Success**: Full patch manipulation capability

### Phase 3: Package (Week 5)
**Goal**: Distributable package

- [ ] Max Package structure
- [ ] Help patch (`maxmcp.maxhelp`)
- [ ] Example patches
- [ ] Documentation

**Success**: Users can install via Package Manager

### Phase 4: Polish (Week 6)
**Goal**: Production ready

- [ ] Cross-platform builds (macOS/Windows)
- [ ] E2E testing
- [ ] Performance optimization
- [ ] Package Manager submission

**Success**: Public release

---

## First Implementation Steps

### 1. Environment Setup
```bash
# Install CMake
brew install cmake

# Install nlohmann/json
brew install nlohmann-json

# Download Max SDK
# https://github.com/Cycling74/max-sdk
```

### 2. Complete Local Setup Checklist

Follow this exact order to reproduce the working installation from our latest debugging session:

1. **Clone repos**
   ```bash
   git clone https://github.com/signalcompose/MaxMCP.git
   cd MaxMCP
   git clone https://github.com/Cycling74/max-sdk.git --recursive max-sdk
   ```
   > The `--recursive` flag is mandatory; without it the `max-pretarget.cmake` script is missing.

2. **Install system dependencies**
   ```bash
   brew install cmake libwebsockets openssl
   ```
   - Requires macOS 13+, Xcode CLT, Max 9.1+, Node 18+, npm 9+.

3. **Install bridge dependencies**
   ```bash
   cd bridge
   npm install
   cd ..
   ```
   Missing this step is what triggered the original `WebSocket error` (the `ws` module was absent).

4. **Build the external**
   ```bash
   ./build.sh Release clean   # optional but recommended for first build
   ./build.sh Release
   ```
   - Copies `maxmcp.mxo` to `~/Documents/Max 9/Library/`.
   - Confirm with `ls ~/Documents/Max\ 9/Library/maxmcp.mxo/Contents/MacOS/maxmcp`.

5. **(Optional) Manual CMake flags**
   ```bash
   cmake -B build -S . -DCMAKE_BUILD_TYPE=Release \
         -DC74_LIBRARY_OUTPUT_DIRECTORY="$HOME/Documents/Max 9/Library"
   cmake --build build --config Release
   ```
   Use this variant if you prefer running `cmake` directly or need a custom Max Library path.

6. **Launch the agent in Max**
   - Unlock a patcher (Cmd+E) and create:
     ```
     [maxmcp @mode agent @port 7400]
     ```
   - Add a `START` message box, connect it to the agent inlet, then click it.
   - Console output should include:
     ```
     WebSocket server started on port 7400
     maxmcp: maxmcp (agent mode) started on port 7400
     ```

7. **Launch a controllable patch**
   - In any patch you want Claude to control, add:
     ```
     [maxmcp @mode patch]
     ```
   - Optional attributes: `@alias my-synth`, `@group synth`.
   - Console will show `Patch registered: <ID>`.

8. **Run the bridge**
   ```bash
   node ~/Documents/MaxMCP/bridge/websocket-mcp-bridge.js ws://localhost:7400
   ```
   - Use `DEBUG=1` for verbose logs.
   - `ECONNREFUSED` means the agent is not running or the port is blocked (`lsof -i :7400` to verify).

9. **Configure Claude Code**
   ```bash
   claude mcp add maxmcp node \
     ~/Documents/MaxMCP/bridge/websocket-mcp-bridge.js \
     ws://localhost:7400
   ```
   Restart Claude Code afterwards.

10. **Smoke test**
    ```bash
    echo '{"jsonrpc":"2.0","method":"tools/list","id":1}' | \
      node bridge/websocket-mcp-bridge.js ws://localhost:7400
    ```
    - Expect a JSON response listing `list_active_patches`, `get_console_log`, etc.
    - For scripted testing without Max, use `node test-mcp-server.js 7400` to spin up the mock server first.

11. **Troubleshooting commands**
    - Port status: `lsof -i :7400`
    - Kill stray bridge: `pkill -f websocket-mcp-bridge.js`
    - Clear stuck agent thread: re-open the patcher and click `START` again.

### 3. Create Minimal External
```cpp
// src/maxmcp.cpp
#include "ext.h"
#include "ext_obex.h"

static t_class* maxmcp_class = nullptr;

typedef struct _maxmcp {
    t_object ob;
} t_maxmcp;

void* maxmcp_new(t_symbol* s, long argc, t_atom* argv) {
    t_maxmcp* x = (t_maxmcp*)object_alloc(maxmcp_class);
    object_post((t_object*)x, "MaxMCP initialized!");
    return x;
}

void ext_main(void* r) {
    t_class* c = class_new("maxmcp",
        (method)maxmcp_new,
        nullptr,
        sizeof(t_maxmcp),
        nullptr,
        A_GIMME,
        0);

    class_register(CLASS_BOX, c);
    maxmcp_class = c;
}
```

### 3. Build with CMake
```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.19)
project(MaxMCP)

set(C74_MAX_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/max-sdk")
include(${C74_MAX_SDK_PATH}/script/max-sdk-base.cmake)

add_library(${PROJECT_NAME} MODULE src/maxmcp.cpp)

include(${C74_MAX_SDK_PATH}/script/max-posttarget.cmake)
```

```bash
cmake -B build -S .
cmake --build build
# Result: build/maxmcp.mxo (macOS) or build/maxmcp.mxe64 (Windows)
```

### 4. Test in Max
1. Copy `build/maxmcp.mxo` to `~/Documents/Max 9/Library/`
2. Create new patch
3. Add object: `[maxmcp]`
4. See "MaxMCP initialized!" in Max console

---

## Important Design Decisions

### Why stdio over Socket.IO?
- **Simpler**: No port management, no Socket.IO dependency
- **Standard**: MCP natively supports stdio
- **Reliable**: Direct communication, no network issues

### Why C++ over JavaScript?
- **Stability**: No V8 engine quirks, no autowatch issues
- **Performance**: Direct Max API access
- **Distribution**: Single .mxo/.mxe64 file, no npm install

### Why auto-generated patch IDs?
- **User Experience**: No manual configuration
- **Flexibility**: Same patch can be opened multiple times
- **Natural Language**: "synth patch" is more intuitive than "synth_main_v2"

---

## Critical Implementation Notes

### Thread Safety
Max API calls **must** be on main thread:
```cpp
// WRONG - crashes
void* some_thread() {
    jpatcher_add_object(patcher, obj); // ❌ Not on main thread
}

// CORRECT
void* some_thread() {
    defer_low(x, [](t_maxmcp* x) {
        jpatcher_add_object(x->patcher, obj); // ✅ On main thread
    }, 0, nullptr);
}
```

### Patcher Reference
Get patcher in `new()`:
```cpp
void* maxmcp_new(...) {
    t_maxmcp* x = (t_maxmcp*)object_alloc(maxmcp_class);
    x->patcher = (t_object*)gensym("#P")->s_thing;
    return x;
}
```

### Lifecycle Monitoring
Subscribe to patcher close event:
```cpp
object_subscribe(gensym("#P"), gensym("close"),
                (method)on_patcher_close, x);
```

---

## Next Steps for Claude Code

1. **Read full specification**: `MAXMCP_V2_DESIGN.md`
2. **Set up development environment**: Max SDK + CMake
3. **Implement Phase 1 MVP**: Basic external + 2 MCP tools
4. **Test with Claude Code**: Verify stdio communication works
5. **Iterate through remaining phases**

---

## Success Criteria

- [ ] Installation: < 30 seconds (Package Manager)
- [ ] Startup: < 3 seconds (patch open → MCP ready)
- [ ] Memory: < 10MB per patch
- [ ] Response: < 100ms per operation
- [ ] Stability: 24-hour continuous operation

---

## Questions?

Refer to:
- **Full spec**: `MAXMCP_V2_DESIGN.md`
- **Max SDK docs**: https://github.com/Cycling74/max-sdk
- **MCP docs**: https://modelcontextprotocol.io/
- **Current implementation** (reference): `/Users/yamato/Src/proj_max_mcp/MaxMSP-MCP-Server-multipatch/`

---

**Ready to start? Read `MAXMCP_V2_DESIGN.md` and begin Phase 1!**
