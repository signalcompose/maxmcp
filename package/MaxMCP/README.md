# MaxMCP - MCP Server for Max/MSP

**Control Max/MSP with natural language using Claude Code**

MaxMCP is a native C++ external that implements the Model Context Protocol (MCP), enabling Claude Code to programmatically control Max/MSP patches through natural language commands.

---

## Features

- 🎛️ **Full Patch Control**: Create, modify, and connect Max objects
- 🔌 **Auto-Discovery**: Patches automatically register with Claude Code
- 🧹 **Zero Configuration**: Just add `[maxmcp]` to your patch
- 🔗 **Connection Management**: Create and remove patchcords programmatically
- 📊 **Patch Introspection**: Query objects, positions, and metadata
- 🎯 **Smart Layout**: Automatic positioning for new objects

---

## Installation

### Method 1: Max Package Manager (Recommended)
1. Open Max/MSP
2. Go to File → Show Package Manager
3. Search for "MaxMCP"
4. Click Install

### Method 2: Manual Installation
1. Download the latest release from [GitHub](https://github.com/signalcompose/MaxMCP/releases)
2. Extract `MaxMCP` folder
3. Copy to `~/Documents/Max 9/Packages/`
4. Restart Max

### Method 3: Development (Symlink)
```bash
ln -s /path/to/MaxMCP/package/MaxMCP ~/Documents/Max\ 9/Packages/MaxMCP
```

---

## Quick Start

### 1. Add MaxMCP to Your Patch

Create a new Max patch and add:
```
[maxmcp synth]
```

The optional argument ("synth") sets the display name. If omitted, the patch filename is used.

### 2. Configure Claude Code

Add MaxMCP server to your Claude Code MCP settings:

```json
{
  "mcpServers": {
    "maxmcp": {
      "command": "/path/to/maxmcp.server.mxo/Contents/MacOS/maxmcp.server",
      "args": []
    }
  }
}
```

### 3. Start Creating

In Claude Code, try:
```
"Create a simple synthesizer with a 440Hz oscillator connected to the left channel of dac~"
```

Claude Code will:
- List your active Max patches
- Create the cycle~ object at position [100, 100]
- Create the dac~ object at position [100, 200]
- Connect cycle~ outlet 0 → dac~ inlet 0

---

## Usage Examples

### Example 1: Create Objects
```
"Add a number box at position [200, 100] named 'freq'"
```
Result: Creates `[number]` object with varname "freq"

### Example 2: Set Attributes
```
"Set the minimum value of 'freq' to 20 and maximum to 2000"
```
Result: Sets `min` and `max` attributes

### Example 3: Make Connections
```
"Connect freq to the frequency inlet of osc1"
```
Result: Creates patchcord from freq → osc1 inlet 0

### Example 4: Build Effect Chain
```
"Create a reverb effect: route audio from synth_out through a reverb~ object to master_out"
```

---

## Available MCP Tools

Claude Code has access to these tools:

### Patch Management
- `list_active_patches()` - List all registered patches
- `get_patch_info(patch_id)` - Get patch metadata
- `get_frontmost_patch()` - Get currently focused patch

### Object Operations
- `add_max_object(patch_id, obj_type, position, varname, arguments)` - Create object
- `remove_max_object(patch_id, varname)` - Remove object
- `set_object_attribute(patch_id, varname, attribute, value)` - Set attribute

### Connection Management
- `connect_max_objects(patch_id, src, outlet, dst, inlet)` - Create patchcord
- `disconnect_max_objects(...)` - Remove patchcord

### Patch Information
- `get_objects_in_patch(patch_id)` - List all objects
- `get_avoid_rect_position(patch_id)` - Find empty space

### Utilities
- `get_console_log(lines, clear)` - Retrieve Max Console messages

---

## Architecture

```
┌─────────────────┐
│   Claude Code   │
│   (MCP Client)  │
└────────┬────────┘
         │ stdio (JSON-RPC)
         ▼
┌─────────────────────┐
│  maxmcp.server.mxo  │  ← MCP Server (singleton)
│  (Max External)     │
└────────┬────────────┘
         │ Global Registry
         ▼
┌──────────────────────────────┐
│  [maxmcp] [maxmcp] [maxmcp]  │  ← Client Objects
│    Patch A   Patch B  Patch C │
└──────────────────────────────┘
```

**Key Components**:
- **maxmcp.server.mxo**: Singleton MCP server, one instance per Max session
- **maxmcp.mxo**: Client object, one instance per patch
- **Global Registry**: Tracks all active patches by auto-generated ID

---

## Patch ID Format

Each patch gets a unique ID when `[maxmcp]` is created:

**Format**: `{patchname}_{uuid8}`

**Examples**:
- `synth.maxpat` → `synth_a7f2b3c1`
- `effect.maxpat` → `effect_d9e4f5a2`
- `Untitled` → `patch_b3c8a1f7`

---

## Troubleshooting

### MaxMCP object not found
**Solution**: Ensure package is installed in `~/Documents/Max 9/Packages/`

### No patches listed in Claude Code
**Solution**:
1. Check `[maxmcp]` object is in your patch
2. Verify maxmcp.server is running (check Max Console)
3. Ensure Claude Code MCP settings are correct

### Objects created in wrong patch
**Solution**: Use `get_frontmost_patch()` or specify exact patch_id

### Connection fails
**Solution**:
1. Verify both objects exist (use `get_objects_in_patch()`)
2. Check varnames are set correctly
3. Verify outlet/inlet indices (0-based)

### Server not responding
**Solution**:
1. Restart Max
2. Check Max Console for errors
3. Verify server external is not corrupted

---

## Development

### Building from Source

**Prerequisites**:
- CMake 3.19+
- Max SDK 8.6+
- Xcode (macOS)
- nlohmann/json

**Build Steps**:
```bash
# Clone repository
git clone https://github.com/signalcompose/MaxMCP.git
cd MaxMCP

# Build server external
cmake -B build -S . -DBUILD_MODE=server -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Build client external
cmake -B build -S . -DBUILD_MODE=client -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Install to package
cmake --install build --prefix package/MaxMCP
```

### Running Tests
```bash
cmake -B build-tests -S . -DBUILD_TESTS=ON
cmake --build build-tests
cd build-tests && ctest --output-on-failure
```

---

## Contributing

Contributions welcome! Please see [CONTRIBUTING.md](https://github.com/signalcompose/MaxMCP/blob/main/CONTRIBUTING.md)

---

## License

MIT License - see [LICENSE](LICENSE) file

---

## Links

- **GitHub**: https://github.com/signalcompose/MaxMCP
- **Issues**: https://github.com/signalcompose/MaxMCP/issues
- **Discussions**: https://github.com/signalcompose/MaxMCP/discussions
- **MCP Protocol**: https://modelcontextprotocol.io/
- **Max SDK**: https://github.com/Cycling74/max-sdk

---

## Credits

**Author**: signalcompose
**Built with**: Max SDK, nlohmann/json, Google Test
**Powered by**: Model Context Protocol (MCP)

---

**🤖 MaxMCP - Bringing AI-assisted patching to Max/MSP**
