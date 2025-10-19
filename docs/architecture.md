# MaxMCP Architecture

**Last Updated**: 2025-10-19
**Status**: Draft

---

## 1. Executive Summary

MaxMCP is a native C++ external object for Max/MSP that implements an MCP (Model Context Protocol) server, enabling natural language control of Max/MSP patches through Claude Code.

---

## 2. System Overview

### 2.1 High-Level Architecture (2-Object Design)

```
┌─────────────────────────────────────────────────────────┐
│              Claude Code (MCP Client)                    │
│  - Natural language processing                          │
│  - Tool orchestration                                    │
│  - Console log monitoring                                │
└────────────────────┬────────────────────────────────────┘
                     │
                     │ stdio (JSON-RPC)
                     │ - Line-based protocol
                     │ - Bidirectional communication
                     │
                ┌────▼─────────┐
                │[maxmcp.server]│ ← Singleton MCP Server
                │  (1 instance) │
                └────┬──────────┘
                     │
        ┌────────────┼────────────┐
        │            │            │
   ┌────▼────┐  ┌───▼────┐  ┌───▼────┐
   │[maxmcp] │  │[maxmcp]│  │[maxmcp]│ ← Client objects
   │ synth   │  │  fx    │  │ master │   (multiple instances)
   └────┬────┘  └───┬────┘  └───┬────┘
        │           │           │
        │ Max API   │           │ Max API
        │           │           │
        ▼           ▼           ▼
   this.patcher  this.patcher  this.patcher
```

**Key Design Decisions**:
- **Two-object architecture**: Separation of server and client responsibilities
- **Singleton server**: Only one `[maxmcp.server]` per Max instance
- **Multiple clients**: Each `[maxmcp]` represents a controllable patch
- **Centralized logging**: All Max Console output captured by server
- **Clear dependency**: Clients cannot exist without server

### 2.2 Component Interaction Flow

```
┌─────────┐       ┌──────────┐       ┌───────────┐       ┌─────────┐
│ Claude  │──1──▶│   stdio  │──2──▶│  maxmcp   │──3──▶│   Max   │
│  Code   │◀──6──│          │◀──5──│  Object   │◀──4──│ Patcher │
└─────────┘       └──────────┘       └───────────┘       └─────────┘

1. Send MCP request (JSON-RPC)
2. Parse request, identify tool
3. Execute Max API call (defer to main thread)
4. Get result from Max
5. Build JSON-RPC response
6. Return response to Claude Code
```

---

## 3. Component Details

### 3.1 Server Object: `[maxmcp.server]`

**Responsibility**: Singleton MCP server, console logging, global patch registry

**Data Structure**:
```cpp
struct t_maxmcp_server {
    t_object ob;                        // Max object header
    void* outlet_log;                   // Outlet for log messages (optional)

    std::unique_ptr<MCPServer> mcp_server;     // MCP protocol handler
    std::unique_ptr<ConsoleLogger> logger;     // Console log capture
};
```

**Singleton Pattern**:
```cpp
static t_maxmcp_server* g_server_instance = nullptr;

void* maxmcp_server_new(...) {
    if (g_server_instance != nullptr) {
        object_error(nullptr, "maxmcp.server already exists!");
        return nullptr;
    }
    // ... create instance
    g_server_instance = x;
    return x;
}
```

**Responsibilities**:
- Start/stop MCP server (stdio communication)
- Capture all Max Console messages
- Maintain global patch registry
- Provide `get_console_log()` MCP tool
- Coordinate tool execution across all patches

**Thread Model**:
```
IO Thread (stdin/stdout)
    ↓
JSON-RPC Request
    ↓
defer_low()
    ↓
Max Main Thread
    ↓
Tool Execution
    ↓
ConsoleLogger::log()
```

---

### 3.2 Client Object: `[maxmcp]`

**Responsibility**: Patch identification and registration

**Data Structure**:
```cpp
struct t_maxmcp {
    t_object ob;                        // Max object header

    std::string patch_id;               // Auto-generated (e.g., "synth_a7f2")
    std::string display_name;           // User-facing name
    t_object* patcher;                  // Pointer to owning patcher

    // Attributes
    t_symbol* alias;                    // Custom patch ID override
    t_symbol* group;                    // Patch group name
};
```

**Server Dependency Check**:
```cpp
void* maxmcp_new(...) {
    if (g_server_instance == nullptr) {
        object_error(nullptr,
            "maxmcp.server not found! Create [maxmcp.server] first");
        return nullptr;
    }
    // ... register with server
}
```

**Lifecycle**:
```
[New]
  ↓
maxmcp_new()
  ↓
Generate patch_id
  ↓
Start MCP server
  ↓
Setup lifecycle monitoring
  ↓
[Registered]
  ↓
(Patch close event)
  ↓
on_patcher_close()
  ↓
Unregister from MCP client
  ↓
maxmcp_free()
  ↓
[Destroyed]
```

**Design Decisions**:
- **Auto-generated patch IDs**: `{patchname}_{uuid_short}` format
- **No user input required**: Zero configuration
- **Automatic cleanup**: Subscribe to close event
- **Single instance per patch**: One `[maxmcp]` object per patcher

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

**Logging Wrapper Macro**:
```cpp
#define MAXMCP_POST(obj, ...) \
    do { \
        char buf[512]; \
        snprintf(buf, sizeof(buf), __VA_ARGS__); \
        ConsoleLogger::log(buf); \
        object_post(obj, "%s", buf); \
    } while(0)
```

**Use Cases**:
- Claude monitors patch operation results
- Debugging tool execution
- Error detection and recovery
- User feedback loop

**Example MCP Tool Response**:
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

---

### 3.4 MCP Tools

**Responsibility**: Implement MCP tool endpoints

**Organization**:
```
src/tools/
├── patch_management.cpp     # list_active_patches, get_patch_info
├── object_operations.cpp    # add/remove/modify objects
├── connection_management.cpp  # connect/disconnect
└── documentation.cpp         # list_all_objects, get_object_doc
```

**Example Tool Implementation**:
```cpp
// tools/object_operations.cpp
json MaxMCP::tool_add_max_object(const json& params) {
    std::string patch_id = params["patch_id"];

    // Validate patch_id
    if (patch_id != patch_id_) {
        return error_response("Patch not managed by this instance");
    }

    // Extract parameters
    auto pos = params["position"];
    auto obj_type = params["obj_type"].get<std::string>();
    auto varname = params["varname"].get<std::string>();

    // Defer to main thread (CRITICAL)
    defer_low(this, [pos, obj_type, varname](MaxMCP* x) {
        // Create object
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

    return {{"status", "success"}, {"varname", varname}};
}
```

**Design Decisions**:
- **Defer all Max API calls**: Max API is **not** thread-safe
- **Validate patch_id**: Each `[maxmcp]` only manages its patcher
- **Error responses**: JSON-RPC error codes
- **Idempotent operations**: Safe to retry

---

### 3.4 Patch ID Generation

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

**Design Decisions**:
- **Human-readable prefix**: Helps with debugging
- **UUID suffix**: Guarantees uniqueness
- **Short UUID**: Balance between uniqueness and readability
- **No manual input**: Automatic on instantiation

**Example IDs**:
```
synth.maxpat      → synth_a7f2b3c9
myeffect.maxpat   → myeffect_b3e1d5a2
Untitled.maxpat   → Untitled_c9d4e6f8
```

---

### 3.5 Fuzzy Matching (Claude Code Side)

**Responsibility**: Resolve natural language to patch_id

**Algorithm**:
```python
def resolve_patch_id(user_input: str) -> str:
    # 1. Get all active patches
    patches = list_active_patches()
    # [
    #   {"patch_id": "synth_a7f2", "display_name": "synth"},
    #   {"patch_id": "fx_b3e1", "display_name": "fx"}
    # ]

    # 2. Fuzzy match against display_name
    candidates = []
    for patch in patches:
        score = fuzz.ratio(user_input, patch["display_name"])
        if score > 70:  # Threshold
            candidates.append((patch, score))

    # 3. Auto-select if unambiguous
    if len(candidates) == 1:
        return candidates[0][0]["patch_id"]
    elif len(candidates) > 1:
        # Ask user to choose
        return ask_user_choice(candidates)
    else:
        raise ValueError(f"No patch matching '{user_input}'")
```

**Design Decisions**:
- **Client-side logic**: Keep external object simple
- **Fuzzy scoring**: Handles typos, abbreviations
- **Threshold**: 70% prevents false positives
- **User confirmation**: Ambiguity handled gracefully

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
[maxmcp] object created
    ↓
maxmcp_new() called
    ↓
patch_id = "synth_a7f2"
    ↓
MCP server started
    ↓
Notification sent: patch_registered
    ↓
Claude Code adds to active_patches
    ↓
---
User closes synth.maxpat
    ↓
Patcher close event
    ↓
on_patcher_close() called
    ↓
Notification sent: patch_unregistered
    ↓
Claude Code removes from active_patches
    ↓
maxmcp_free() called
    ↓
MCP server stopped
```

---

## 5. Design Rationale

### 5.1 Why C++ over JavaScript?

| Aspect | JavaScript (Old) | C++ (New) |
|--------|------------------|-----------|
| Stability | V8 engine quirks, autowatch issues | Direct Max API, no runtime |
| Performance | Interpreter overhead | Native code |
| Distribution | 6 files + npm dependencies | Single .mxo/.mxe64 |
| Debugging | Hard to attach debugger | Standard C++ tools |
| Max API | Through JS wrapper | Direct access |

**Decision**: C++ for stability, performance, simplicity.

---

### 5.2 Why stdio over Socket.IO?

| Aspect | Socket.IO (Old) | stdio (New) |
|--------|-----------------|-------------|
| Setup | Port management, firewall | None |
| Dependencies | socket.io npm package | None |
| Protocol | Custom events | Standard JSON-RPC |
| Reliability | Network stack | Pipe (OS-level) |
| Debugging | Wireshark, packet capture | Simple logging |

**Decision**: stdio for simplicity, reliability.

---

### 5.3 Why Auto-Generated patch_id?

**Alternative 1: Manual input**
```
❌ [maxmcp my_synth_patch_v2]  # User must type this
```

**Problems**:
- Error-prone (typos)
- Doesn't support multiple instances (same name collision)
- Requires user to think about naming

**Alternative 2: Auto-generated (chosen)**
```
✅ [maxmcp]  # No arguments
   → patch_id = "synth_a7f2"
```

**Benefits**:
- Zero configuration
- Supports multiple instances
- Human-readable prefix
- Unique suffix

**Decision**: Auto-generated for UX, reliability.

---

## 6. Security Considerations

### 6.1 Threat Model

| Threat | Mitigation |
|--------|-----------|
| Malicious MCP commands | Validate all inputs, no shell execution |
| Patch corruption | Validate operations, atomic transactions |
| Memory exhaustion | Limit object count, monitor memory |
| Infinite loops | Timeout on operations |

### 6.2 Input Validation

All MCP tool calls validate:
- `patch_id` matches this instance
- Object names are alphanumeric
- Positions are within bounds
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

### 7.2 Memory Footprint

- **Base**: ~5MB (MCP server, JSON library)
- **Per patch**: ~2MB (object metadata cache)
- **Total (10 patches)**: ~25MB (acceptable)

---

## 8. Failure Modes

### 8.1 Known Limitations

1. **Max API threading**: All calls must be on main thread
2. **stdio buffering**: Large responses may be chunked
3. **Patcher references**: Weak pointers, check before use

### 8.2 Error Handling Strategy

```cpp
json execute_tool(const std::string& tool, const json& params) {
    try {
        // Validate input
        if (!validate_params(params)) {
            return error_response("Invalid parameters");
        }

        // Execute tool
        return dispatch_tool(tool, params);
    }
    catch (const std::exception& e) {
        return error_response(e.what());
    }
    catch (...) {
        return error_response("Unknown error");
    }
}
```

**Principles**:
- Never throw across thread boundaries
- Always return JSON-RPC error response
- Log errors to Max console
- Fail gracefully

---

## 9. Future Architecture Considerations

### 9.1 Potential Extensions (Post-v2.0)

1. **Network MCP**: WebSocket server for remote control
2. **Multi-client**: Multiple Claude Code instances
3. **Distributed patches**: Patches across multiple Max instances
4. **Preset management**: Save/load patch states
5. **Undo/Redo**: MCP-aware undo stack

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
- [Old Implementation](../MaxMSP-MCP-Server-multipatch/)

---

**This architecture follows DDD (Documentation Driven Development) and serves as the blueprint for MaxMCP v2.0 implementation.**
