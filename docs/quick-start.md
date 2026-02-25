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
Node.js Bridge (websocket-mcp-bridge.js)
    ↕ WebSocket
[maxmcp @mode agent] C++ External (MCP Server)
    ↕ Max API
[maxmcp @mode patch] Registered Patches
```

### Architecture Approach
- ✅ **Unified C++ external object** (`@mode agent` / `@mode patch`)
- ✅ **WebSocket + stdio bridge** (Node.js bridge translates stdio to WebSocket)
- ✅ **26 MCP tools** across 6 categories

---

## Key Features

### 1. Auto-Generated Patch IDs
```
User places:  [maxmcp @mode patch]
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
- **Language**: C/C++17 (Max SDK 8.6+)
- **Build System**: CMake
- **MCP Protocol**: stdio ↔ WebSocket (via Node.js bridge)
- **JSON Library**: nlohmann/json
- **WebSocket**: libwebsockets
- **Testing**: Google Test
- **Distribution**: Max Package

### Rationale
- **C++**: Most stable, direct Max API access, no external dependencies
- **stdio + WebSocket bridge**: MCP uses stdio natively; bridge translates to WebSocket for Max communication
- **Max Package**: Best user experience (Package Manager install)

---

## Project Structure

```
MaxMCP/
├── src/                         # C++ source code
│   ├── maxmcp.cpp              # Unified external (@mode agent / @mode patch)
│   ├── mcp_server.cpp          # MCP protocol handler
│   ├── websocket_server.cpp    # WebSocket server
│   ├── tools/                  # MCP tool implementations (7 files, 26 tools)
│   └── utils/                  # Shared utilities (4 files)
├── tests/unit/                 # Google Test unit tests
├── package/MaxMCP/             # Max Package (build output)
│   └── support/bridge/         # Node.js stdio-to-WebSocket bridge
├── build.sh                    # Build script
├── deploy.sh                   # Deploy to Max 9 Packages
└── CMakeLists.txt              # Build configuration
```

For detailed source file organization, see [development-guide.md](development-guide.md) §4.1.

---

## Development Phases

### Phase 1: MVP ✅ Complete
**Goal**: Basic functioning external object

- [x] Max SDK setup and CMake build system
- [x] Unified `[maxmcp]` external (`@mode agent` / `@mode patch`)
- [x] WebSocket communication (JSON-RPC over WebSocket)
- [x] 3 core MCP tools (`list_active_patches`, `add_max_object`, `get_console_log`)
- [x] Unit testing framework (Google Test)

### Phase 2: Complete MCP Toolset ✅ Complete
**Goal**: Full patch manipulation capability

- [x] Auto-generated patch IDs
- [x] Lifecycle monitoring (close → unregister)
- [x] 10 MCP tools
- [x] stdio-to-WebSocket bridge (Node.js)
- [x] E2E testing with Claude Code
- [x] Max Package structure

### Post-Phase 2: Tool Expansion ✅ Complete
**Goal**: Comprehensive tool coverage

- [x] 26 MCP tools across 6 categories
- [x] Claude Code plugin with 4 skills
- [x] Build/deploy scripts (`build.sh`, `deploy.sh`)

### Phase 3: Package Distribution (Planned)
- [ ] Max Package Manager submission
- [ ] GitHub releases
- [ ] Documentation website

### Phase 4: Cross-Platform (Planned)
- [ ] Windows build (x64)
- [ ] Intel Mac build (x86_64)

---

## First Implementation Steps

### 1. Environment Setup
```bash
# Install dependencies
brew install cmake nlohmann-json libwebsockets openssl

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
   brew install cmake nlohmann-json libwebsockets openssl
   ```
   - Requires macOS 13+, Xcode CLT, Max 9.1+, Node 18+, npm 9+.

3. **Install bridge dependencies**
   ```bash
   cd package/MaxMCP/support/bridge
   npm install
   cd ../../../..
   ```

4. **Build the external**
   ```bash
   ./build.sh --clean Release   # optional but recommended for first build
   ./build.sh Release
   ```
   - Installs `maxmcp.mxo` to `package/MaxMCP/externals/`.

5. **Deploy to Max 9 Packages**
   ```bash
   ./deploy.sh
   ```
   - Copies `package/MaxMCP/` to `~/Documents/Max 9/Packages/MaxMCP`.

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
   node ~/Documents/Max\ 9/Packages/MaxMCP/support/bridge/websocket-mcp-bridge.js ws://localhost:7400
   ```
   - Use `DEBUG=1` for verbose logs.
   - `ECONNREFUSED` means the agent is not running or the port is blocked (`lsof -i :7400` to verify).

9. **Configure Claude Code**
   ```bash
   claude mcp add maxmcp node \
     ~/Documents/Max\ 9/Packages/MaxMCP/support/bridge/websocket-mcp-bridge.js \
     ws://localhost:7400
   ```
   Restart Claude Code afterwards.

10. **(Optional) Install Claude Code Plugin**
    ```bash
    /plugin marketplace add signalcompose/maxmcp
    /plugin install maxmcp@maxmcp
    ```
    **Skills**:
    - `/maxmcp:patch-guidelines` - Patch creation guidelines
    - `/maxmcp:max-techniques` - Max/MSP implementation techniques
    - `/maxmcp:m4l-techniques` - Max for Live development techniques
    - `/maxmcp:max-resources` - Access Max.app documentation

11. **Smoke test**
    ```bash
    echo '{"jsonrpc":"2.0","method":"tools/list","id":1}' | \
      node package/MaxMCP/support/bridge/websocket-mcp-bridge.js ws://localhost:7400
    ```
    - Expect a JSON response listing all 26 MCP tools.

12. **Troubleshooting commands**
    - Port status: `lsof -i :7400`
    - Kill stray bridge: `pkill -f websocket-mcp-bridge.js`
    - Clear stuck agent thread: re-open the patcher and click `START` again.

---

## Important Design Decisions

### Why stdio + WebSocket Bridge?
- **Standard**: MCP natively uses stdio; bridge adds no protocol complexity
- **Reliable**: WebSocket provides persistent connection to Max
- **Flexible**: Bridge can run locally or remotely

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

- **Thread Safety**: All Max API calls must run on main thread via `defer()` / `defer_low()`
- **Patcher Reference**: Obtained at instantiation via `gensym("#P")->s_thing`
- **Lifecycle Monitoring**: Subscribe to patcher close event for auto-unregistration

For code examples and detailed architecture, see [architecture.md](architecture.md) §3.

---

## Next Steps for Claude Code

1. **Read full specification**: [specifications.md](specifications.md)
2. **See all MCP tools**: [mcp-tools-reference.md](mcp-tools-reference.md)
3. **Understand architecture**: [architecture.md](architecture.md)
4. **Development practices**: [development-guide.md](development-guide.md)

---

## Success Criteria

- [ ] Installation: < 30 seconds (Package Manager)
- [ ] Startup: < 3 seconds (patch open → MCP ready)
- [ ] Memory: < 10MB per patch
- [ ] Response: < 100ms per operation
- [ ] Stability: 24-hour continuous operation

---

## References

- **Max SDK docs**: https://github.com/Cycling74/max-sdk
- **MCP docs**: https://modelcontextprotocol.io/
- **Documentation index**: [INDEX.md](INDEX.md)
