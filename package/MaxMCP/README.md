# MaxMCP - MCP Server for Max/MSP

**Control Max/MSP with natural language using Claude Code**

MaxMCP is a native C++ external that implements the Model Context Protocol (MCP), enabling Claude Code to programmatically control Max/MSP patches through natural language commands.

---

## Features

- 🎛️ **Full Patch Control**: Create, modify, and connect Max objects
- 🔌 **Auto-Discovery**: Patches automatically register with the MCP server
- 🧹 **Zero Configuration**: Just add `[maxmcp @mode patch]` to your patch
- 🔗 **Connection Management**: Create and remove patchcords programmatically
- 📊 **Patch Introspection**: Query objects, positions, and metadata
- 🎯 **Smart Layout**: Automatic positioning and layout validation

---

## Requirements

- **Max 9.0+** (Apple Silicon / arm64 macOS, 13.0+)
- **Node.js 18+** with npm — required by the WebSocket bridge that connects Claude Code to Max
- **Claude Code** (the MCP client)

> libwebsockets and OpenSSL are bundled into `maxmcp.mxo` at build time, so no extra C/C++ runtime install is required.

---

## Installation

### Method 1: Max Package Manager (Coming Soon)
1. Open Max/MSP
2. Go to File → Show Package Manager
3. Search for "MaxMCP"
4. Click Install

### Method 2: Manual Installation
1. Download the latest release from [GitHub](https://github.com/signalcompose/MaxMCP/releases)
2. Extract the `MaxMCP` folder
3. Copy it to `~/Documents/Max 9/Packages/`
4. Restart Max

> The bridge's Node.js dependencies are installed from `00-index.maxpat` (STEP 1) the first time you run it — no manual `npm install` required.

---

## Architecture

```
Claude Code (MCP Client)
    ↕ stdio (JSON-RPC)
Node.js Bridge (websocket-mcp-bridge.js, launched by Claude Code)
    ↕ WebSocket (ws://localhost:7400)
[maxmcp @mode agent] C++ External Object
    ↕ Max API
[maxmcp @mode patch] ... registered Max patches
```

**Key Components**:
- **maxmcp.mxo**: A single unified external with two modes
  - `@mode agent` — WebSocket server + MCP protocol handler (one per Max instance)
  - `@mode patch` — Patch registration (one per controllable patch, default mode)
- **Bridge**: `support/bridge/websocket-mcp-bridge.js` translates stdio ↔ WebSocket (Node.js). Claude Code launches it automatically once configured.

---

## Quick Start

Open the index patch and follow its four steps:

**In Max: Help → MaxMCP Package → `00-index.maxpat`**, or:
```bash
open ~/Documents/Max\ 9/Packages/MaxMCP/examples/00-index.maxpat
```

### STEP 1 — Install Node.js Dependencies (first time only)
Click the install button in the patch. It runs `npm install` for the bridge via `node.script` (`support/bridge/npm-install.js`).

### STEP 2 — Start the MaxMCP Agent
Click the **`start`** message connected to `[maxmcp @mode agent]`. The Max Console should log that the WebSocket server started on port 7400. (Send `stop` to shut it down.)

### STEP 3 — Configure Claude Code
Run the command shown in the patch:
```bash
claude mcp add maxmcp node ~/Documents/Max\ 9/Packages/MaxMCP/support/bridge/websocket-mcp-bridge.js ws://localhost:7400
```
Restart Claude Code to apply. Claude Code launches the bridge for you on connect.

### STEP 4 — Create / Open a Controllable Patch
Use the buttons in the patch to open the test patches, or add to your own patch:
```
[maxmcp @alias my-synth @group synth]
```

Attributes:
- `@mode patch` — Client mode (default, can be omitted)
- `@alias my-synth` — Custom patch ID (optional, auto-generated if omitted)
- `@group synth` — Group name for filtering (optional)

### Then: Control with Natural Language
In Claude Code, try:
```
"Create a simple synthesizer with a 440Hz oscillator connected to the left channel of dac~"
```

---

## Available MCP Tools

MaxMCP provides **30 tools** across 7 categories:

| Category | Count | Tools |
|----------|-------|-------|
| Patch Management | 3 | `list_active_patches`, `get_patch_info`, `get_frontmost_patch` |
| Object Operations | 12 | `add_max_object`, `remove_max_object`, `get_objects_in_patch`, `set_object_attribute`, `get_object_attribute`, `get_object_value`, `get_object_io_info`, `get_object_hidden`, `set_object_hidden`, `redraw_object`, `replace_object_text`, `assign_varnames` |
| Connection Operations | 4 | `connect_max_objects`, `disconnect_max_objects`, `get_patchlines`, `set_patchline_midpoints` |
| Patch State | 3 | `get_patch_lock_state`, `set_patch_lock_state`, `get_patch_dirty` |
| Hierarchy | 2 | `get_parent_patcher`, `get_subpatchers` |
| Utilities | 2 | `get_console_log`, `get_avoid_rect_position` |
| Layout Validation | 4 | `validate_layout`, `get_io_position`, `suggest_alignment`, `align_objects` |

See the [full tool reference](https://github.com/signalcompose/MaxMCP/blob/main/docs/mcp-tools-reference.md) for parameters and response formats.

---

## Example Patches

Included in `examples/`:

- **00-index.maxpat** — Main index / launcher (the four-step onboarding above)
- **01-basic-registration.maxpat** — Basic patch registration
- **01-claude-code-connection.maxpat** — Claude Code E2E connection test
- **02-custom-alias.maxpat** — Custom alias usage
- **03-group-assignment.maxpat** — Group filtering
- **04-05-06-multi-patch-*.maxpat** — Multi-patch scenarios (synth1, synth2, fx1)
- **07-mcp-tools-test.maxpat** — MCP tools testing

---

## Troubleshooting

### Agent won't start
- Check that port 7400 is available: `lsof -i :7400`
- Verify the `start` message reached `[maxmcp @mode agent]`
- Check the Max Console for error messages

### Claude Code can't connect
- Make sure STEP 1 (bridge `npm install`) completed successfully
- Confirm the agent is running and logged a successful start
- Restart Claude Code after `claude mcp add`; verify with `claude mcp list`
- Capture WebSocket errors:
  `DEBUG=1 node ~/Documents/Max\ 9/Packages/MaxMCP/support/bridge/websocket-mcp-bridge.js ws://localhost:7400`

### Objects not appearing
- Ensure the patch is unlocked (Cmd+E)
- Confirm the patch has a `[maxmcp @mode patch]` object
- Verify the patch ID with `list_active_patches`

---

## License

MaxMCP is licensed under the **Signal compose Fair Trade License v1.0** — see [LICENSE](LICENSE).

Third-party attributions: see [THIRD_PARTY_LICENSES.md](https://github.com/signalcompose/MaxMCP/blob/main/THIRD_PARTY_LICENSES.md).

---

## Links

- **GitHub**: https://github.com/signalcompose/MaxMCP
- **Issues**: https://github.com/signalcompose/MaxMCP/issues
- **MCP Protocol**: https://modelcontextprotocol.io/
- **Max SDK**: https://github.com/Cycling74/max-sdk

---

## Credits

**Author**: Signal compose
**Built with**: Max SDK, libwebsockets, OpenSSL, nlohmann/json, ws (Node.js)
**Powered by**: Model Context Protocol (MCP)

---

**🤖 MaxMCP - Bringing AI-assisted patching to Max/MSP**
