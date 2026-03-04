# MaxMCP Architecture

**Last Updated**: 2026-02-22
**Status**: 26 MCP Tools Implemented

---

## 1. Executive Summary

MaxMCP is a native C++ external object for Max/MSP that implements an MCP (Model Context Protocol) server, enabling natural language control of Max/MSP patches through Claude Code.

---

## 2. System Overview

### 2.1 High-Level Architecture (Unified External + Bridge)

```
┌─────────────────────────────────────────────────────────┐
│              Claude Code (MCP Client)                    │
│  - Natural language processing                          │
│  - Tool orchestration                                    │
│  - Console log monitoring                                │
└────────────────────┬────────────────────────────────────┘
                     │
                     │ stdio (JSON-RPC)
                     │
            ┌────────▼─────────┐
            │  Node.js Bridge  │ ← websocket-mcp-bridge.js
            │  (stdio ↔ WS)   │
            └────────┬─────────┘
                     │
                     │ WebSocket (JSON-RPC)
                     │
         ┌───────────▼─────────────┐
         │ [maxmcp @mode agent]    │ ← Singleton MCP Server
         │  - WebSocket server     │
         │  - MCP protocol handler │
         │  - Patch registry       │
         │  - Console logger       │
         └───────────┬─────────────┘
                     │
        ┌────────────┼────────────┐
        │            │            │
   ┌────▼─────┐ ┌───▼─────┐ ┌───▼─────┐
   │[maxmcp   │ │[maxmcp  │ │[maxmcp  │ ← Client objects
   │@mode     │ │@mode    │ │@mode    │   (multiple instances)
   │ patch]   │ │ patch]  │ │ patch]  │
   │ synth    │ │  fx     │ │ master  │
   └────┬─────┘ └───┬─────┘ └───┬─────┘
        │           │           │
        │ Max API   │           │ Max API
        ▼           ▼           ▼
   this.patcher  this.patcher  this.patcher
```

**Key Design Decisions**:
- **Unified external**: Single `maxmcp.mxo` with `@mode` attribute selects role
- **Agent singleton**: Only one `[maxmcp @mode agent]` per Max instance
- **Multiple clients**: Each `[maxmcp @mode patch]` represents a controllable patch
- **WebSocket + Bridge**: Node.js bridge translates MCP stdio to WebSocket for Max communication
- **Centralized logging**: All Max Console output captured by agent

### 2.2 Component Interaction Flow

```
┌─────────┐       ┌──────────┐       ┌──────────┐       ┌───────────┐       ┌─────────┐
│ Claude  │──1──▶│  Bridge  │──2──▶│  Agent   │──3──▶│   Patch   │──4──▶│   Max   │
│  Code   │◀──7──│  (Node)  │◀──6──│  Mode    │◀──5──│   Mode    │◀─────│ Patcher │
└─────────┘       └──────────┘       └──────────┘       └───────────┘       └─────────┘

1. Send MCP request (JSON-RPC over stdio)
2. Forward to WebSocket
3. Parse request, route to tool
4. Execute Max API call (defer to main thread)
5. Get result from Max
6. Build JSON-RPC response
7. Return response to Claude Code
```

---

## 3. Component Details

### 3.1 Agent Mode: `[maxmcp @mode agent]`

**Responsibility**: WebSocket MCP server, console logging, global patch registry

**Key Attributes**:
- `@port` (default: 7400) — WebSocket server port
- `@mode agent` — Activates server mode

**Data Structure** (see `src/maxmcp.h`):
```cpp
typedef struct _maxmcp {
    t_object ob;                        // Max object header (must be first)

    // Mode selection
    t_symbol* mode;                     // "agent" or "patch" (@mode attribute)

    // === AGENT MODE FIELDS ===
    void* outlet_log;                   // Outlet for log messages
    bool initialized;                   // MCP initialize handshake completed
    std::string protocol_version;       // Negotiated protocol version
    std::atomic<bool> running;          // Server running state
    WebSocketServer* ws_server;         // WebSocket server instance
    t_atom_long port;                   // WebSocket server port (@port attribute)
    bool debug;                         // Debug mode (@debug attribute)

    // === PATCH MODE FIELDS ===
    std::string patch_id;               // Auto-generated unique ID
    std::string display_name;           // User-friendly name for patch
    std::string patcher_name;           // Max patcher filename
    t_object* patcher;                  // Reference to parent patcher object
    t_symbol* alias;                    // Custom patch ID override (@alias attribute)
    t_symbol* group;                    // Patch group name (@group attribute)
} t_maxmcp;
```

> **Note**: Legacy source files `maxmcp_server.cpp` and `udp_server.cpp` exist in the
> repository but are not included in the current build. They are from an earlier architecture
> where agent and client were separate externals.

**Singleton Pattern**:
```cpp
static t_maxmcp* g_agent_instance = nullptr;

void* maxmcp_new(...) {
    // ...
    if (mode == gensym("agent")) {
        if (g_agent_instance != nullptr) {
            object_error(nullptr, "maxmcp agent already exists!");
            return nullptr;
        }
        g_agent_instance = x;
    }
    // ...
}
```

**Responsibilities**:
- Start/stop WebSocket MCP server
- Capture all Max Console messages
- Maintain global patch registry
- Provide `get_console_log()` MCP tool
- Coordinate tool execution across all registered patches

**Thread Model**:
```
WebSocket Thread (libwebsockets)
    ↓
JSON-RPC Request
    ↓
defer()
    ↓
Max Main Thread
    ↓
Tool Execution
    ↓
WebSocket Response
```

---

### 3.2 Patch Mode: `[maxmcp @mode patch]`

**Responsibility**: Patch identification and registration

**Key Attributes**:
- `@mode patch` — Activates client mode
- `@alias` (optional) — Custom patch name override
- `@group` (optional) — Patch group for filtering

**Lifecycle**:
```
[New]
  ↓
maxmcp_new(@mode patch)
  ↓
Generate patch_id from patcher filename + UUID
  ↓
Register with agent's patch registry
  ↓
[Registered]
  ↓
(Patch close event)
  ↓
on_patcher_close()
  ↓
Unregister from patch registry
  ↓
maxmcp_free()
  ↓
[Destroyed]
```

**Design Decisions**:
- **Auto-generated patch IDs**: `{patchname}_{uuid_short}` format
- **No user input required**: Zero configuration
- **Automatic cleanup**: Subscribe to patcher close event
- **No server dependency at instantiation**: Patch objects register when agent is available

---

### 3.3 Console Logger

**Responsibility**: Capture and buffer Max Console output for Claude Code

**Implementation**:
```cpp
class ConsoleLogger {
private:
    static std::deque<std::string> log_buffer_;
    static const size_t MAX_BUFFER_SIZE = 1000;
    static std::mutex mutex_;

public:
    static void log(const char* message);
    static json get_logs(size_t count = 50, bool clear = false);
    static void clear();
};
```

**Ring Buffer Design**:
- Maximum 1000 log lines in memory
- Oldest entries automatically discarded
- Thread-safe access with mutex
- Available via `get_console_log()` MCP tool

---

### 3.4 MCP Tools

**Responsibility**: Implement 26 MCP tool endpoints across 6 categories

**Organization**:
```
src/tools/
├── patch_tools.cpp      # list_active_patches, get_patch_info, get_frontmost_patch
├── object_tools.cpp     # add/remove/modify/query objects (12 tools)
├── connection_tools.cpp # connect/disconnect/get_patchlines/set_midpoints
├── state_tools.cpp      # lock state, dirty state
├── hierarchy_tools.cpp  # parent patcher, subpatchers
├── utility_tools.cpp    # console log, avoid rect position
└── tool_common.cpp      # Shared helpers (find object by varname, etc.)
```

For the complete tool reference with parameters and response formats, see [mcp-tools-reference.md](mcp-tools-reference.md).

**Design Decisions**:
- **Defer all Max API calls**: Max API is **not** thread-safe
- **Validate patch_id**: Each tool validates the target patch exists
- **Error responses**: JSON-RPC error codes
- **Idempotent operations**: Safe to retry

---

### 3.5 Patch ID Generation

**Responsibility**: Create unique, human-readable patch identifiers

**Algorithm**:
```cpp
std::string generate_patch_id(t_object* patcher) {
    // Get patcher filename (e.g., "synth.maxpat")
    t_symbol* patchname = jpatcher_get_name(patcher);

    // Remove extension → "synth"
    std::string name = remove_extension(patchname->s_name);

    // Generate short UUID (8 chars)
    std::string uuid = generate_short_uuid(8);

    // Combine → "synth_a7f2b3c9"
    return name + "_" + uuid;
}
```

**Example IDs**:
```
synth.maxpat      → synth_a7f2b3c9
myeffect.maxpat   → myeffect_b3e1d5a2
Untitled.maxpat   → Untitled_c9d4e6f8
```

---

### 3.6 Fuzzy Matching (Claude Code Side)

**Responsibility**: Resolve natural language to patch_id

Claude Code handles fuzzy matching on the client side:
1. Get all active patches via `list_active_patches()`
2. Match user input against `display_name` fields
3. Auto-select if unambiguous, ask user if ambiguous

**Design Decision**: Client-side logic keeps the external object simple.

---

## 4. Data Flow

### 4.1 Tool Call Flow

```
Claude Code:
  "Add a 440Hz oscillator to synth patch"
    ↓
  1. list_active_patches()
     Response: [{"patch_id": "synth_a7f2", ...}]
    ↓
  2. Fuzzy match "synth" → "synth_a7f2"
    ↓
  3. add_max_object(
       patch_id="synth_a7f2",
       obj_type="cycle~",
       args=[440],
       position=[100, 100],
       varname="osc1"
     )
     Response: {"status": "success"}
    ↓
  Max Patch: [cycle~ 440] appears at (100, 100)
```

### 4.2 Lifecycle Event Flow

```
User opens synth.maxpat
    ↓
[maxmcp @mode patch] object created
    ↓
maxmcp_new() called
    ↓
patch_id = "synth_a7f2"
    ↓
Register with agent's patch registry
    ↓
---
User closes synth.maxpat
    ↓
Patcher close event
    ↓
on_patcher_close() called
    ↓
Unregister from patch registry
    ↓
maxmcp_free() called
```

### 4.3 Communication Flow (stdio ↔ WebSocket ↔ Max)

```
┌────────────┐
│ Claude Code│
│(MCP Client)│
└─────┬──────┘
      │ stdin/stdout (JSON-RPC, line-delimited)
      │
┌─────▼──────────────────────────────────┐
│ websocket-mcp-bridge.js (Node.js)     │
│ - Reads stdin line by line             │
│ - Forwards JSON-RPC to WebSocket      │
│ - Receives WebSocket response          │
│ - Writes to stdout                     │
└─────┬──────────────────────────────────┘
      │ WebSocket (ws://localhost:7400)
      │
┌─────▼──────────────────────────────────┐
│ maxmcp.mxo @mode agent                │
│                                        │
│ ┌────────────────────────────────────┐ │
│ │ WebSocketServer (libwebsockets)   │ │
│ │ - Accept WS connections           │ │
│ │ - Parse JSON-RPC                  │ │
│ │ - defer() to Max main thread      │ │
│ └──────────────┬─────────────────────┘ │
│                │                       │
│                ▼                       │
│ ┌────────────────────────────────────┐ │
│ │ MCPServer (main thread)           │ │
│ │ - Route to tool handler           │ │
│ │ - Execute Max API calls           │ │
│ │ - Build JSON-RPC response         │ │
│ └──────────────┬─────────────────────┘ │
│                │                       │
│                ▼ send via WebSocket     │
└────────────────────────────────────────┘
```

**Key Design Points**:

1. **Thread Safety**:
   - WebSocket thread receives messages asynchronously
   - `defer()` ensures Max API calls run on main thread
   - Response sent back via WebSocket from main thread

2. **Line-Based Protocol**:
   - Each JSON-RPC message is a single line (stdio side)
   - WebSocket messages contain one JSON-RPC request/response

3. **Error Handling**:
   - JSON parse errors return JSON-RPC error response
   - Tool execution errors caught and returned as JSON
   - Server continues running on individual request failures

---

## 5. Design Rationale

### 5.1 Why C++ over JavaScript?

| Aspect | JavaScript (Old) | C++ (Current) |
|--------|------------------|-----------|
| Stability | V8 engine quirks, autowatch issues | Direct Max API, no runtime |
| Performance | Interpreter overhead | Native code |
| Distribution | Multiple files + npm dependencies | Single .mxo/.mxe64 |
| Debugging | Hard to attach debugger | Standard C++ tools |
| Max API | Through JS wrapper | Direct access |

**Decision**: C++ for stability, performance, simplicity.

---

### 5.2 Why WebSocket + stdio Bridge?

| Aspect | Socket.IO (Old) | stdio + WebSocket (Current) |
|--------|-----------------|-------------|
| MCP compatibility | Custom events | Native MCP stdio protocol |
| Max communication | N/A | WebSocket (persistent, bidirectional) |
| Dependencies | socket.io npm | ws (lightweight) |
| Setup | Port + Socket.IO config | Bridge auto-connects |

**Decision**: stdio for MCP compatibility, WebSocket for reliable Max communication via bridge.

---

### 5.3 Why Auto-Generated patch_id?

**Alternative 1: Manual input** — Error-prone, doesn't support multiple instances.

**Alternative 2: Auto-generated (chosen)** — Zero configuration, supports multiple instances, human-readable prefix + unique suffix.

**Decision**: Auto-generated for UX, reliability.

---

## 6. Security Considerations

### 6.1 Threat Model

| Threat | Mitigation |
|--------|-----------|
| Malicious MCP commands | Validate all inputs, no shell execution |
| Patch corruption | Validate operations before execution |
| Memory exhaustion | Limit log buffer, monitor memory |
| Unauthorized access | Local-only WebSocket (localhost) |

### 6.2 Input Validation

All MCP tool calls validate:
- `patch_id` exists in registry
- Object varnames exist in target patch
- Positions are within reasonable bounds
- Connection endpoints exist

---

## 7. Performance Characteristics

### 7.1 Benchmarks (Target)

| Operation | Target | Notes |
|-----------|--------|-------|
| Startup | < 3 seconds | From patch open to MCP ready |
| list_active_patches | < 50ms | 10 patches |
| add_max_object | < 100ms | Includes UI update |
| connect_objects | < 80ms | Includes UI update |
| get_objects_in_patch | < 100ms | 50 objects |
| assign_varnames | < 100ms | 50 assignments |

### 7.2 Memory Footprint

- **Base**: ~5MB (MCP server, WebSocket server, JSON library)
- **Per patch**: ~2MB (object metadata)
- **Total (10 patches)**: ~25MB (acceptable)

---

## 8. Failure Modes

### 8.1 Known Limitations

1. **Max API threading**: All calls must be on main thread
2. **WebSocket connection**: Bridge must be running for Claude Code communication
3. **Patcher references**: Must check validity before use

### 8.2 Error Handling Strategy

**Principles**:
- Never throw across thread boundaries
- Always return JSON-RPC error response
- Log errors to Max console
- Fail gracefully

---

## 9. Future Architecture Considerations

### 9.1 Potential Extensions

1. **Multi-client**: Multiple Claude Code instances via WebSocket
2. **Distributed patches**: Patches across multiple Max instances
3. **Preset management**: Save/load patch states
4. **Undo/Redo**: MCP-aware undo stack

### 9.2 Scalability

Current architecture supports:
- **Patches**: Unlimited (memory-bound)
- **Objects per patch**: 1000+ (tested)
- **Connections**: 500+ (tested)
- **Concurrent operations**: Serialized (Max main thread)

---

## 10. References

- [Max SDK Documentation](https://github.com/Cycling74/max-sdk)
- [MCP Specification](https://spec.modelcontextprotocol.io/)
- [nlohmann/json](https://github.com/nlohmann/json)
- [libwebsockets](https://libwebsockets.org/)
- [MCP Tools Reference](mcp-tools-reference.md)

---

**This architecture follows DDD (Documentation Driven Development) and serves as the blueprint for MaxMCP implementation.**
