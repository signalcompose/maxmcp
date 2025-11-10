# MaxMCP WebSocket Bridge

WebSocket bridge that translates stdio MCP (Claude Code) to WebSocket (maxmcp.server).

This bridge is packaged as a single executable binary using [pkg](https://github.com/vercel/pkg), containing:
- Node.js v18 runtime
- websocket-mcp-bridge.js
- node_modules/ws

**No Node.js installation required!**

---

## Binary Information

- **File**: `dist/maxmcp-bridge`
- **Size**: ~44MB
- **Architecture**: arm64 (Apple Silicon)
- **Platform**: macOS

---

## Usage

### Basic Usage

```bash
# Connect to local maxmcp.server
./dist/maxmcp-bridge ws://localhost:7400

# Connect with authentication
./dist/maxmcp-bridge ws://localhost:7400 secret-token-123

# Connect to remote server
./dist/maxmcp-bridge wss://remote.example.com:7400 auth-token
```

### Claude Code Configuration

Add to `~/.claude.json`:

```json
{
  "mcpServers": {
    "maxmcp": {
      "type": "stdio",
      "command": "/Users/yamato/Documents/Max 9/Packages/MaxMCP/bin/maxmcp-bridge",
      "args": ["ws://localhost:7400"]
    }
  }
}
```

**With Authentication**:
```json
{
  "mcpServers": {
    "maxmcp": {
      "type": "stdio",
      "command": "/Users/yamato/Documents/Max 9/Packages/MaxMCP/bin/maxmcp-bridge",
      "args": ["ws://localhost:7400", "your-auth-token"]
    }
  }
}
```

**Remote Connection**:
```json
{
  "mcpServers": {
    "maxmcp-remote": {
      "type": "stdio",
      "command": "/Users/yamato/Documents/Max 9/Packages/MaxMCP/bin/maxmcp-bridge",
      "args": ["wss://remote.example.com:7400", "your-auth-token"]
    }
  }
}
```

---

## Debug Mode

Enable debug logging by setting the `DEBUG` environment variable:

```bash
DEBUG=1 ./dist/maxmcp-bridge ws://localhost:7400
```

This will output debug messages to stderr, including:
- Connection status
- Message send/receive events
- Error details

---

## Building from Source

### Prerequisites

- Node.js 18+ (for building only)
- npm

### Build Steps

```bash
# Install dependencies
npm install

# Build binary
./build.sh
```

The binary will be created at `dist/maxmcp-bridge`.

### Manual Build

```bash
# Install dependencies
npm install

# Install pkg globally
npm install -g pkg

# Build
pkg . --targets node18-macos-arm64 --output dist/maxmcp-bridge
```

---

## Architecture

```
Claude Code (stdio MCP Client)
    ↕ stdin/stdout (JSON-RPC 2.0)
maxmcp-bridge (this binary)
    ├─ Node.js v18 runtime
    ├─ websocket-mcp-bridge.js
    └─ node_modules/ws
    ↕ WebSocket (ws://localhost:7400)
maxmcp.server.mxo (Max external object)
    ↕ Max API
Max Patches
```

---

## Error Handling

### Connection Failed

```
WebSocket error: connect ECONNREFUSED 127.0.0.1:7400
```

**Solution**: Ensure maxmcp.server is running in Max

### Authentication Failed

```
WebSocket connection closed
```

**Solution**: Check auth token matches maxmcp.server @auth attribute

### Invalid URL

```
SyntaxError: Invalid URL: <url>
```

**Solution**: Ensure WebSocket URL starts with `ws://` or `wss://`

---

## Development

### Source Files

- `websocket-mcp-bridge.js` - Main bridge implementation
- `package.json` - Dependencies and pkg configuration
- `build.sh` - Build script

### Testing

```bash
# Test with local server (requires maxmcp.server running)
echo '{"jsonrpc":"2.0","method":"tools/list","id":1}' | ./dist/maxmcp-bridge ws://localhost:7400

# Expected output: JSON-RPC response from maxmcp.server
```

---

## License

MIT

---

## See Also

- [MaxMCP Documentation](../docs/INDEX.md)
- [maxmcp.server Usage](../docs/server-usage.md)
- [pkg Documentation](https://github.com/vercel/pkg)
