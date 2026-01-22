#!/usr/bin/env node
/**
 * MaxMCP WebSocket Bridge
 *
 * Translates stdio (Claude Code MCP) to WebSocket (maxmcp.server)
 * This file is packaged into a single binary using pkg.
 *
 * Usage:
 *   maxmcp-bridge [options] [ws_url] [auth_token]
 *   maxmcp-bridge --port 18081
 *   maxmcp-bridge ws://localhost:7400
 *   maxmcp-bridge wss://remote:7400 secret-token-123
 *
 * Options:
 *   --port <number>  WebSocket port (default: 18081)
 *   --host <string>  WebSocket host (default: localhost)
 */

const WebSocket = require('ws');
const readline = require('readline');

// Parse command line arguments
let wsUrl = null;
let authToken = null;
let wsPort = 18081;  // Default port
let wsHost = 'localhost';

// Process arguments
for (let i = 2; i < process.argv.length; i++) {
  const arg = process.argv[i];

  if (arg === '--port' && i + 1 < process.argv.length) {
    wsPort = parseInt(process.argv[++i], 10);
  } else if (arg === '--host' && i + 1 < process.argv.length) {
    wsHost = process.argv[++i];
  } else if (arg.startsWith('ws://') || arg.startsWith('wss://')) {
    wsUrl = arg;
    // Get auth token if provided
    if (i + 1 < process.argv.length && !process.argv[i + 1].startsWith('--')) {
      authToken = process.argv[++i];
    }
    break;
  }
}

// Construct URL if not provided
if (!wsUrl) {
  wsUrl = `ws://${wsHost}:${wsPort}`;
}

// Debug mode (set DEBUG=1 environment variable)
const DEBUG = process.env.DEBUG === '1';

function debug(...args) {
  if (DEBUG) {
    console.error('[Bridge Debug]', ...args);
  }
}

function formatError(err) {
  if (!err) {
    return 'Unknown error';
  }

  const parts = [];

  if (err.message) {
    parts.push(err.message.trim());
  }
  if (err.code) {
    parts.push(`code=${err.code}`);
  }
  if (typeof err.errno !== 'undefined') {
    parts.push(`errno=${err.errno}`);
  }
  if (err.address) {
    parts.push(`address=${err.address}`);
  }
  if (typeof err.port !== 'undefined') {
    parts.push(`port=${err.port}`);
  }

  if (err instanceof AggregateError && Array.isArray(err.errors) && err.errors.length > 0) {
    const innerMessages = err.errors.map((inner) => formatError(inner));
    parts.push(`inner=[${innerMessages.join('; ')}]`);
  }

  if (parts.length === 0 && err.stack) {
    return err.stack;
  }

  return parts.join(' ');
}

function logWebSocketError(err) {
  const formatted = formatError(err);
  console.error('WebSocket error:', formatted);
  if (DEBUG && err && err.stack) {
    console.error(err.stack);
  }
}

debug('Starting MaxMCP WebSocket Bridge');
debug('WebSocket URL:', wsUrl);
debug('Auth Token:', authToken ? '***' : 'none');

// WebSocket connection with optional authentication
// Node.js 'ws' library: WebSocket(url, protocols, options)
// protocols can be string, array, or included in options
const wsOptions = {};
if (authToken) {
  wsOptions.headers = {
    'Authorization': `Bearer ${authToken}`
  };
}

const ws = new WebSocket(wsUrl, 'mcp', wsOptions);

// Setup readline for stdin
const rl = readline.createInterface({
  input: process.stdin,
  output: process.stdout,
  terminal: false
});

// Buffer for messages received before WebSocket is connected
let messageBuffer = [];
let isConnected = false;

// Connection established
ws.on('open', () => {
  debug('WebSocket connection established');
  isConnected = true;

  // Send any buffered messages
  while (messageBuffer.length > 0) {
    const bufferedMessage = messageBuffer.shift();
    debug('Sending buffered message to WebSocket:', bufferedMessage.substring(0, 100));
    ws.send(bufferedMessage);
  }
});

// stdin → WebSocket (with buffering)
rl.on('line', (line) => {
  if (isConnected && ws.readyState === WebSocket.OPEN) {
    debug('Sending to WebSocket:', line.substring(0, 100));
    ws.send(line);
  } else {
    debug('Buffering message (WebSocket not ready):', line.substring(0, 100));
    messageBuffer.push(line);
  }
});

// WebSocket → stdout
ws.on('message', (data) => {
  const message = data.toString();
  debug('Received from WebSocket:', message.substring(0, 100));
  console.log(message);
});

// Error handling
ws.on('error', (err) => {
  logWebSocketError(err);
  process.exit(1);
});

ws.on('close', (code, reason) => {
  const reasonText = reason ? reason.toString() : '';
  debug('WebSocket connection closed:', code, reasonText);
  if (code !== 1000) {
    console.error('WebSocket connection closed unexpectedly:', code, reasonText);
  } else {
    console.error('WebSocket connection closed');
  }
  process.exit(0);
});

// Graceful shutdown
process.on('SIGINT', () => {
  debug('Received SIGINT, closing connection');
  ws.close();
  process.exit(0);
});

process.on('SIGTERM', () => {
  debug('Received SIGTERM, closing connection');
  ws.close();
  process.exit(0);
});
