# MaxMCP - Complete Design Specification

**Version**: 1.0.0-alpha
**Last Updated**: 2025-10-19
**Status**: Phase 1 MVP Complete

> **Comprehensive specification for new project**
> A new Claude Code instance can read this document and immediately begin implementation.

---

## 📋 Project Overview

### Mission
Develop a native MCP server external object for Max/MSP, enabling Claude Code to control Max/MSP patches using natural language.

### Goals
1. **One-click installation**: Via Max Package Manager
2. **Zero configuration**: Works immediately after placing `[maxmcp]`
3. **Full automation**: Auto-generated patch IDs, lifecycle management
4. **Natural language**: "Add oscillator to synth patch" just works

### Tech Stack (Confirmed)
- **C/C++** (Max SDK 8.6+)
- **WebSocket** (libwebsockets 4.4.1)
- **Node.js Bridge** (stdio-to-WebSocket translation)
- **CMake** (Cross-platform builds)
- **JSON** (nlohmann/json)

---

## 🏗️ System Architecture

### Overall Diagram
```
┌─────────────────────────────────────────────────────┐
│              Claude Code (MCP Client)                │
└────────────────────┬────────────────────────────────┘
                     │ stdio (JSON-RPC)
         ┌───────────┴───────────┐
         │                       │
    ┌────▼────┐            ┌────▼────┐
    │[maxmcp] │            │[maxmcp] │
    │synth.maxpat│         │fx.maxpat│
    └─────────┘            └─────────┘
         │                       │
         ▼                       ▼
    this.patcher           this.patcher
```

### Architecture Approach

**Two-Component Design**: Separation of server and bridge responsibilities.

- **maxmcp.server.mxo**: WebSocket server handling JSON-RPC over WebSocket
- **websocket-mcp-bridge.js**: stdio-to-WebSocket bridge (Node.js)

### Communication Protocol

**WebSocket-based MCP JSON-RPC**:

```
Claude Code (stdio MCP)
    ↓ stdin/stdout
websocket-mcp-bridge.js (Node.js)
    ↓ WebSocket (ws://localhost:7400)
maxmcp.server.mxo (C++ / libwebsockets)
    ↓ Max API
Max/MSP Patches
```

### WebSocket Server (maxmcp.server.mxo)

**Library**: libwebsockets 4.4.1
**Protocol**: WebSocket over TCP
**Default Port**: 7400 (chosen to avoid conflicts with other MCP servers like Serena)
**Test Port**: 7401 (used in unit tests)
**Message Format**: JSON-RPC 2.0 (text frames)

**Connection Flow**:
1. Client connects to `ws://localhost:7400` or `wss://remote:7400`
2. Optional authentication via `Authorization: Bearer <token>` header
3. Bidirectional JSON-RPC message exchange
4. Server maintains connection until client disconnects or error occurs

**Multi-Client Support**:
- Server accepts multiple simultaneous WebSocket connections
- Each client receives a unique client_id (UUID)
- Requests are queued (FIFO) to ensure atomic patch operations
- Thread-safe client list management

**Example Request (WebSocket text frame)**:
```json
{"jsonrpc":"2.0","method":"tools/call","params":{"name":"add_max_object","arguments":{"patch_id":"synth_a7f2","obj_type":"cycle~","x":100,"y":100,"varname":"osc1","arguments":[440]}},"id":1}
```

**Example Response (WebSocket text frame)**:
```json
{"jsonrpc":"2.0","result":{"status":"success","patch_id":"synth_a7f2","varname":"osc1"},"id":1}
```

**Error Response**:
```json
{"jsonrpc":"2.0","error":{"code":-32700,"message":"Parse error"},"id":null}
```

**Authentication**:
- Optional token-based authentication
- Configured via `@auth` attribute: `[maxmcp.server @port 7400 @auth "secret-token"]`
- Client must send `Authorization: Bearer secret-token` header
- Unauthenticated connections are rejected with close code 1008 (Policy Violation)

**Thread Safety**:
1. libwebsockets event loop runs in background thread
2. qelem defers JSON processing to Max main thread
3. All Max API calls occur on main thread only
4. Client list protected by mutex

### WebSocket MCP Bridge (websocket-mcp-bridge.js)

**Purpose**: Translate stdio MCP (Claude Code) to WebSocket (Max)

**Implementation**:
```javascript
// stdin (Claude Code) → WebSocket (Max)
process.stdin.on('data', (data) => {
  ws.send(data.toString());
});

// WebSocket (Max) → stdout (Claude Code)
ws.on('message', (data) => {
  console.log(data.toString());
});
```

**Usage**:
```bash
node websocket-mcp-bridge.js ws://localhost:7400 [auth-token]
```

**MCP Server Configuration** (`~/.claude.json`):
```json
{
  "mcpServers": {
    "maxmcp": {
      "type": "stdio",
      "command": "node",
      "args": [
        "/path/to/websocket-mcp-bridge.js",
        "ws://localhost:7400"
      ]
    }
  }
}
```

**Remote Access Example**:
```json
{
  "mcpServers": {
    "maxmcp-remote": {
      "type": "stdio",
      "command": "node",
      "args": [
        "/path/to/websocket-mcp-bridge.js",
        "wss://gallery.example.com:7400",
        "secret-token"
      ]
    }
  }
}
```

---

## 🎯 Key Features

### 1. Auto-Generated Patch IDs

#### Specification
```
patch_id = "{patchname}_{uuid_short}"
Examples: "synth_a7f2", "myeffect_b3e1", "master_c9d4"
```

#### Implementation
```cpp
class MaxMCP : public MaxCppObject<MaxMCP> {
private:
    std::string patch_id_;
    std::string display_name_;
    t_object* patcher_;

    std::string generate_patch_id() {
        t_symbol* patchname = jpatcher_get_name(patcher_);
        std::string name = remove_extension(patchname->s_name);
        std::string uuid = generate_short_uuid(8);
        return name + "_" + uuid;
    }

public:
    void init() {
        patcher_ = (t_object*)gensym("#P")->s_thing;
        display_name_ = get_patcher_display_name();
        patch_id_ = generate_patch_id();

        post("MaxMCP: Initialized patch_id=%s", patch_id_.c_str());
    }
};
```

#### User Experience
```
Before: [js mcp_client.js my_synth_patch_v2]  ← Manual input
After:  [maxmcp]                               ← No arguments!
```

---

### 2. Lifecycle Management

#### Auto-Registration/Unregistration
```cpp
class MaxMCP {
    void setup_lifecycle_monitoring() {
        // On loadbang: auto-register
        register_with_mcp_client();

        // Monitor patcher close
        object_attach_byptr_register(this, patcher_, CLASS_NOBOX);
        object_subscribe(gensym("#P"), gensym("close"),
                        (method)on_patcher_close, this);
    }

    static void on_patcher_close(MaxMCP* x, t_symbol* s, long argc, t_atom* argv) {
        x->unregister_from_mcp_client();
        x->cleanup();
    }

    void register_with_mcp_client() {
        // Send registration notification to MCP client
        send_notification("patch_registered", {
            {"patch_id", patch_id_},
            {"display_name", display_name_},
            {"file_path", get_patcher_filepath()}
        });
    }
};
```

#### State Transitions
```
[New] → loadbang → [Registered] → close → [Unregistered] → [Destroyed]
```

---

### 3. MCP Tools Implementation

#### Required Tools List

**Phase 1 MVP (Implemented)** ✅:
1. **Console Logging**
   - ✅ `get_console_log(lines, clear)` - Retrieve Max Console messages

2. **Patch Management**
   - ✅ `list_active_patches()` - List active patches

3. **Object Operations**
   - ✅ `add_max_object(patch_id, obj_type, position, varname)` - Add Max object to patch

**Phase 2 (Planned)**:
4. **Patch Management (Extended)**
   - `get_patch_info(patch_id)` - Get patch details
   - `get_frontmost_patch()` - Get frontmost patch

5. **Object Operations (Extended)**
   - `remove_max_object(patch_id, varname)`
   - `set_object_attribute(patch_id, varname, attr_name, value)`

6. **Connection Management**
   - `connect_max_objects(patch_id, src, outlet, dst, inlet)`
   - `disconnect_max_objects(patch_id, src, outlet, dst, inlet)`

7. **Patch Information**
   - `get_objects_in_patch(patch_id)`
   - `get_avoid_rect_position(patch_id)`

8. **Documentation** (Optional)
   - `list_all_objects()`
   - `get_object_doc(object_name)`

#### Console Logging Tool Specification

**Tool**: `get_console_log`

**Description**: Retrieve recent Max Console messages to monitor patch state and debug operations

**Parameters**:
```json
{
  "lines": {
    "type": "number",
    "description": "Number of recent log lines (default: 50, max: 1000)",
    "optional": true
  },
  "filter": {
    "type": "string",
    "description": "Regex filter for log messages (optional)",
    "optional": true
  },
  "clear": {
    "type": "boolean",
    "description": "Clear log after reading (default: false)",
    "optional": true
  }
}
```

**Response**:
```json
{
  "logs": [
    "MaxMCP: Created cycle~ at [100, 100]",
    "MaxMCP: Set varname to osc1",
    "MaxMCP: Connected osc1[0] -> dac~[0]",
    "dsp: audio on"
  ],
  "count": 4
}
```

**Use Cases**:
- Monitor object creation results
- Detect connection errors
- Debug tool execution
- Verify patch state changes
- Provide feedback to Claude for decision making

#### MCP Tool Implementation Example
```cpp
// tools/add_max_object.cpp
json MaxMCP::tool_add_max_object(const json& params) {
    std::string patch_id = params["patch_id"];

    // Check if this is our patch_id
    if (patch_id != patch_id_) {
        return error_response("Patch not managed by this instance");
    }

    // Execute on main thread
    defer_low(this, [params](MaxMCP* x) {
        auto pos = params["position"];
        auto obj_type = params["obj_type"].get<std::string>();
        auto varname = params["varname"].get<std::string>();

        t_object* obj = (t_object*)object_new_typed(
            CLASS_BOX,
            gensym(obj_type.c_str()),
            0, nullptr
        );

        // Set position
        t_rect rect;
        rect.x = pos[0].get<double>();
        rect.y = pos[1].get<double>();
        object_attr_setrect(obj, gensym("patching_rect"), &rect);

        // Set varname
        object_attr_setsym(obj, gensym("varname"), gensym(varname.c_str()));

        // Add to patcher
        jpatcher_add_object(x->patcher_, obj);

    }, 0, nullptr);

    return {{"status", "success"}, {"varname", params["varname"]}};
}
```

---

### 4. stdio-based MCP Server

#### Communication Protocol
```
Claude Code → stdin  → [maxmcp]
Claude Code ← stdout ← [maxmcp]
```

#### JSON-RPC Implementation
```cpp
class MCPServer {
private:
    std::thread io_thread_;
    std::atomic<bool> running_{false};
    MaxMCP* max_object_;

public:
    void start() {
        running_ = true;
        io_thread_ = std::thread([this]() {
            while (running_) {
                std::string line;
                if (!std::getline(std::cin, line)) break;

                auto request = json::parse(line);
                auto response = handle_request(request);

                std::cout << response.dump() << "\n" << std::flush;
            }
        });
    }

    json handle_request(const json& req) {
        std::string method = req["method"];

        if (method == "tools/list") {
            return list_tools();
        }
        else if (method == "tools/call") {
            std::string tool_name = req["params"]["name"];
            json tool_params = req["params"]["arguments"];

            return execute_tool(tool_name, tool_params);
        }

        return error_response("Unknown method");
    }

    json execute_tool(const std::string& tool, const json& params) {
        // Tool dispatch
        if (tool == "add_max_object") {
            return max_object_->tool_add_max_object(params);
        }
        // ... other tools
    }
};
```

---

### 5. Fuzzy Matching (Claude Code Side)

#### Resolving patch_id from Natural Language
```python
# User: "Add oscillator to synth patch"

# 1. Get active patches
patches = list_active_patches()
# [
#   {"patch_id": "synth_a7f2", "display_name": "synth"},
#   {"patch_id": "fx_b3e1", "display_name": "fx"}
# ]

# 2. Fuzzy match
candidates = fuzzy_match("synth", patches)
# → [{"patch_id": "synth_a7f2", "score": 1.0}]

# 3. Auto-select
if len(candidates) == 1:
    patch_id = candidates[0]["patch_id"]
    add_max_object(patch_id=patch_id, ...)
elif len(candidates) > 1:
    # Ask user to choose
    ask_user_to_choose(candidates)
else:
    # Not found
    error("No patch matching 'synth' found")
```

---

## 📦 Max Package Structure

### Directory Layout
```
MaxMCP/
├── init/
│   └── maxmcp.txt              # "max objectfile maxmcp"
├── externals/
│   ├── maxmcp.mxo              # macOS Universal (arm64/x64)
│   └── maxmcp.mxe64            # Windows 64-bit
├── docs/
│   └── refpages/
│       └── maxmcp.maxref.xml   # Max Reference integration
├── help/
│   └── maxmcp.maxhelp          # Help patch
├── examples/
│   ├── 01-getting-started.maxpat
│   ├── 02-synth-creation.maxpat
│   ├── 03-multi-patch-orchestration.maxpat
│   └── 04-effect-chain.maxpat
├── extras/
│   └── docs.json               # Max object documentation (6MB)
├── media/
│   ├── icon.png
│   └── screenshots/
├── support/
│   └── maxmcp-helper.js        # Helper scripts (if needed)
├── patchers/
│   └── maxmcp-inspector.maxpat # Debug tool
├── package-info.json
├── README.md
└── LICENSE
```

### package-info.json
```json
{
  "name": "MaxMCP",
  "version": "1.0.0",
  "author": "Hiroshi Yamato",
  "description": "Native MCP server for Max/MSP - Control your patches with Claude Code using natural language",
  "website": "https://github.com/dropcontrol/MaxMCP",
  "max_version_min": "900",
  "max_version_max": "none",
  "os": {
    "macOS": {
      "min_version": "10.15"
    },
    "windows": {
      "min_version": "10"
    }
  },
  "tags": ["ai", "mcp", "automation", "llm", "claude", "assistant"],
  "filelist": {
    "externals": ["maxmcp.mxo", "maxmcp.mxe64"],
    "help": ["maxmcp.maxhelp"],
    "docs": ["refpages/maxmcp.maxref.xml"]
  }
}
```

### maxmcp.maxref.xml
```xml
<?xml version="1.0" encoding="utf-8"?>
<c74object name="maxmcp">
  <digest>MCP Server for Claude Code integration</digest>
  <description>
    MaxMCP enables Claude Code to control Max/MSP patches using natural language.
    Place this object in your patch and Claude Code can automatically detect and
    manipulate it through the Model Context Protocol.
  </description>

  <metadatalist>
    <metadata name="author">Hiroshi Yamato</metadata>
    <metadata name="tag">ai</metadata>
    <metadata name="tag">automation</metadata>
  </metadatalist>

  <attributelist>
    <attribute name="alias" get="1" set="1" type="symbol" size="1">
      <digest>Custom patch identifier</digest>
      <description>Override auto-generated patch_id with custom name</description>
    </attribute>

    <attribute name="group" get="1" set="1" type="symbol" size="1">
      <digest>Patch group name</digest>
      <description>Assign patch to a group for batch operations</description>
    </attribute>
  </attributelist>

  <seealsolist>
    <seealso name="js"/>
    <seealso name="node.script"/>
  </seealsolist>
</c74object>
```

---

## 🔧 Development Environment Setup

### Required Tools
```bash
# macOS
brew install cmake
brew install nlohmann-json

# Max SDK
curl -O https://cycling74.com/downloads/max-sdk
```

### Project Initialization
```bash
mkdir MaxMCP
cd MaxMCP

# Create CMake project
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Installation target
# macOS: ~/Documents/Max 9/Packages/MaxMCP/
# Windows: %USERPROFILE%\Documents\Max 9\Packages\MaxMCP\
```

### CMakeLists.txt
```cmake
cmake_minimum_required(VERSION 3.19)
project(MaxMCP)

set(CMAKE_CXX_STANDARD 17)

# Max SDK
set(C74_MAX_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/max-sdk")
include(${C74_MAX_SDK_PATH}/script/max-sdk-base.cmake)

# nlohmann/json
find_package(nlohmann_json REQUIRED)

# Source files
set(SOURCES
    src/maxmcp.cpp
    src/mcp_server.cpp
    src/tools/patch_management.cpp
    src/tools/object_operations.cpp
    src/tools/connection_management.cpp
    src/tools/documentation.cpp
    src/utils/json_helpers.cpp
    src/utils/uuid_generator.cpp
)

# External object
add_library(${PROJECT_NAME} MODULE ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)

# Max SDK configuration
include(${C74_MAX_SDK_PATH}/script/max-posttarget.cmake)
```

---

## 📝 Source Code Structure

### File Organization
```
src/
├── maxmcp.cpp              # Main entry point
├── maxmcp.h
├── mcp_server.cpp          # MCP server implementation
├── mcp_server.h
├── tools/
│   ├── patch_management.cpp    # Patch management tools
│   ├── object_operations.cpp   # Object operation tools
│   ├── connection_management.cpp
│   └── documentation.cpp
├── utils/
│   ├── json_helpers.cpp
│   ├── uuid_generator.cpp
│   └── defer_helpers.cpp       # Max main thread defer
└── docs/
    └── docs_database.cpp       # docs.json embedding

tests/
├── test_mcp_server.cpp
├── test_tools.cpp
└── mock_max_api.h

examples/
├── 01-getting-started.maxpat
├── 02-synth-creation.maxpat
└── ...
```

### maxmcp.cpp (Main)
```cpp
#include "ext.h"
#include "ext_obex.h"
#include "jpatcher_api.h"
#include "maxmcp.h"
#include "mcp_server.h"

static t_class* maxmcp_class = nullptr;

struct t_maxmcp {
    t_object ob;

    std::string patch_id;
    std::string display_name;
    t_object* patcher;

    std::unique_ptr<MCPServer> mcp_server;

    // Attributes
    t_symbol* alias;
    t_symbol* group;
};

void* maxmcp_new(t_symbol* s, long argc, t_atom* argv) {
    t_maxmcp* x = (t_maxmcp*)object_alloc(maxmcp_class);

    // Get patcher
    x->patcher = (t_object*)gensym("#P")->s_thing;

    // Generate patch ID
    x->patch_id = generate_patch_id(x->patcher);
    x->display_name = get_patcher_name(x->patcher);

    // Start MCP server
    x->mcp_server = std::make_unique<MCPServer>(x);
    x->mcp_server->start();

    // Setup lifecycle monitoring
    setup_lifecycle_monitoring(x);

    object_post((t_object*)x, "MaxMCP initialized: %s", x->patch_id.c_str());

    return x;
}

void maxmcp_free(t_maxmcp* x) {
    if (x->mcp_server) {
        x->mcp_server->stop();
    }
}

void ext_main(void* r) {
    t_class* c = class_new("maxmcp",
        (method)maxmcp_new,
        (method)maxmcp_free,
        sizeof(t_maxmcp),
        nullptr,
        A_GIMME,
        0);

    // Attributes
    CLASS_ATTR_SYM(c, "alias", 0, t_maxmcp, alias);
    CLASS_ATTR_SYM(c, "group", 0, t_maxmcp, group);

    class_register(CLASS_BOX, c);
    maxmcp_class = c;
}
```

---

## 🧪 Testing Strategy

### Unit Tests (C++)
```cpp
// tests/test_patch_id_generation.cpp
TEST(PatchIDGeneration, BasicName) {
    auto id = generate_patch_id("synth.maxpat");
    EXPECT_TRUE(id.starts_with("synth_"));
    EXPECT_EQ(id.length(), 11); // "synth" + "_" + 5char uuid
}

TEST(PatchIDGeneration, UniquenessGuarantee) {
    auto id1 = generate_patch_id("test.maxpat");
    auto id2 = generate_patch_id("test.maxpat");
    EXPECT_NE(id1, id2); // Different even with same name
}
```

### E2E Tests
```python
# tests/e2e/test_basic_workflow.py
def test_patch_auto_registration():
    """Auto-registration on patch open"""
    # 1. Open Max patch (containing [maxmcp])
    open_max_patch("test.maxpat")

    # 2. Verify from Claude Code
    patches = list_active_patches()
    assert any("test" in p["display_name"] for p in patches)

def test_patch_auto_unregistration():
    """Auto-unregistration on patch close"""
    open_max_patch("test.maxpat")
    patch_id = get_patch_id_by_name("test")

    close_max_patch("test.maxpat")

    patches = list_active_patches()
    assert not any(p["patch_id"] == patch_id for p in patches)
```

---

## 📚 Documentation

### README.md
```markdown
# MaxMCP - Native MCP Server for Max/MSP

Control your Max/MSP patches with Claude Code using natural language.

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

## Features

- ✅ Zero configuration
- ✅ Automatic patch detection
- ✅ Natural language control
- ✅ Multi-patch support
- ✅ Auto-cleanup on close

## Examples

See `examples/` folder for:
- Basic synthesis
- Multi-patch orchestration
- Effect chains
- And more!

## Support

- GitHub Issues: https://github.com/dropcontrol/MaxMCP/issues
- Documentation: https://maxmcp.dev
```

---

## 🚀 Roadmap

### Phase 1: MVP (Week 1-2)
- [ ] Max SDK environment setup
- [ ] Basic external object creation
- [ ] stdio MCP communication
- [ ] `add_max_object()`, `list_active_patches()` implementation

### Phase 2: Core Features (Week 3-4)
- [ ] All MCP tools implementation
- [ ] Auto-generated patch IDs
- [ ] Lifecycle management
- [ ] docs.json integration

### Phase 3: Package (Week 5)
- [ ] Max Package structure creation
- [ ] Help patch creation
- [ ] Example patches creation
- [ ] Documentation finalization

### Phase 4: Polish (Week 6)
- [ ] Cross-platform builds
- [ ] E2E testing
- [ ] Performance optimization
- [ ] Package Manager registration application

---

## 📊 Success Metrics

1. **Installation time**: < 30 seconds (via Package Manager)
2. **Startup time**: < 3 seconds (patch open to MCP ready)
3. **Memory usage**: < 10MB per patch
4. **Response time**: < 100ms (add_object etc.)
5. **Stability**: 24-hour continuous operation without crashes

---

## 🎓 References

### Max SDK
- [Max SDK Documentation](https://github.com/Cycling74/max-sdk)
- [Max API Reference](https://cycling74.com/sdk/max-sdk-8.0.3/html/)

### MCP
- [Model Context Protocol](https://modelcontextprotocol.io/)
- [MCP SDK (C++ bindings TBD)]

### Current Implementation (Reference)
- Current multipatch implementation: `/Users/yamato/Src/proj_max_mcp/MaxMSP-MCP-Server-multipatch/`

---

## ✅ Next Steps

**Commands to execute in new project**:

```bash
# 1. Create project
mkdir MaxMCP && cd MaxMCP
git init

# 2. Get Max SDK
# Download from https://github.com/Cycling74/max-sdk

# 3. Initialize CMake project
mkdir src tests examples
touch CMakeLists.txt

# 4. Start with minimal implementation
# Create src/maxmcp.cpp
```

**Instructions for Claude Code**:
"Implement MaxMCP according to this design specification. Start with Phase 1 MVP."

---

**Place this design document as CLAUDE.md or README in the new project, and a fresh Claude Code instance can immediately begin work.**
