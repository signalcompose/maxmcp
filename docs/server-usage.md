# MaxMCP Server Usage Guide

**Last Updated**: 2026-02-22

---

## Overview

The `[maxmcp]` external runs in two modes:

- `@mode agent` – Spins up the WebSocket MCP server inside Max on a specific port (default 7400). Only one agent per Max instance.
- `@mode patch` – Registers an individual patch so Claude can discover and manipulate it. One per patch you want to expose.

Claude Code connects through the Node.js bridge (`package/MaxMCP/support/bridge/websocket-mcp-bridge.js`), which forwards stdio JSON-RPC to that WebSocket server.

---

## Launching the Agent

1. Unlock a new patcher (Cmd+E) and add:
   ```
   [maxmcp @mode agent @port 7400]
   ```
2. Add a `START` message box and patch it into the agent inlet.
3. Click `START`. The Max Console should log:
   ```
   WebSocket server started on port 7400
   maxmcp: maxmcp (agent mode) started on port 7400
   maxmcp: WebSocket server listening on port 7400
   ```
4. Verify the port is open:
   ```bash
   lsof -i :7400
   ```

---

## Registering Patches

Inside each controllable patch, add:
```
[maxmcp @mode patch @alias my-synth @group synth]
```
- `@alias` (optional) sets a stable name Claude can reference.
- `@group` (optional) allows Claude to filter a family of patches.

Console output will show:
```
Patch registered: MyPatch_ab12Cd34
maxmcp: maxmcp (patch mode) initialized (ID: MyPatch_ab12Cd34)
```

Closing the patch automatically unregisters it.

---

## Running the Bridge

1. Install dependencies once:
   ```bash
   cd package/MaxMCP/support/bridge
   npm install
   ```
2. Start the bridge (stdio ↔ WebSocket):
   ```bash
   node package/MaxMCP/support/bridge/websocket-mcp-bridge.js ws://localhost:7400
   ```
   - Add an auth token as the next argument if your agent requires it.
   - Use verbose mode for troubleshooting:
     ```bash
     DEBUG=1 node package/MaxMCP/support/bridge/websocket-mcp-bridge.js ws://localhost:7400
     ```
   - If the bridge prints `ECONNREFUSED`, start or restart the agent patcher.

3. Smoke test without Claude:
   ```bash
   echo '{"jsonrpc":"2.0","method":"tools/list","id":1}' | \
     node package/MaxMCP/support/bridge/websocket-mcp-bridge.js ws://localhost:7400
   ```
   Successful output lists the available tools.

---

## Claude Code Configuration

```bash
claude mcp add maxmcp node \
  ~/Documents/Max\ 9/Packages/MaxMCP/support/bridge/websocket-mcp-bridge.js \
  ws://localhost:7400
```

Restart Claude Code. Ask:
```
List all active Max patches
```

Expected response lists any `[maxmcp @mode patch]` instances currently registered.

---

## Communication Protocol Summary

The bridge speaks standard MCP (JSON-RPC over stdio). The Max agent exposes 26 MCP tools via WebSocket across 6 categories: Patch Management, Object Operations, Connection Operations, Patch State, Hierarchy, and Utilities.

For a complete tool reference with parameters and response formats, see [MCP Tools Reference](./mcp-tools-reference.md).

Invoke tools through Claude Code or by piping JSON requests into the bridge as shown above.

---

## Troubleshooting

| Symptom | Fix |
| --- | --- |
| Bridge prints `WebSocket error: ECONNREFUSED` | Agent patch not running or wrong port. Re-open `[maxmcp @mode agent @port 7400]`, click `START`, verify with `lsof -i :7400`. |
| Bridge exits without message | Run with `DEBUG=1` to surface the full error (aggregate errors include nested causes). |
| `Patch registered` immediately followed by `Patch unregistered` | The patch was closed or reloaded. Keep it open while testing, or disable auto-close scripts. |
| Claude says “There are currently no active Max patches” | Ensure at least one patch contains `[maxmcp @mode patch]` and the agent is running. Use `tools/list` via CLI to double-check. |
| Port already in use | `lsof -i :7400` then `kill <pid>` or change the agent/bridge command to another port (must match on both sides). |

---

## Reference

For deeper architecture details see `docs/architecture.md`, and for development/testing workflows consult `docs/development-guide.md`.
