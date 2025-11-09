#!/usr/bin/env node
/**
 * MaxMCP WebSocket Bridge
 *
 * Translates stdio (Claude Code MCP) to WebSocket (maxmcp.server)
 * This file is packaged into a single binary using pkg.
 *
 * Usage:
 *   maxmcp-bridge [ws_url] [auth_token]
 *   maxmcp-bridge ws://localhost:7400
 *   maxmcp-bridge wss://remote:7400 secret-token-123
 */

const WebSocket = require('ws');
const readline = require('readline');

// Parse command line arguments
const wsUrl = process.argv[2] || 'ws://localhost:7400';
const authToken = process.argv[3];

// Debug mode (set DEBUG=1 environment variable)
const DEBUG = process.env.DEBUG === '1';

function debug(...args) {
  if (DEBUG) {
    console.error('[Bridge Debug]', ...args);
  }
}

debug('Starting MaxMCP WebSocket Bridge');
debug('WebSocket URL:', wsUrl);
debug('Auth Token:', authToken ? '***' : 'none');

// WebSocket connection with optional authentication
const wsOptions = {};
if (authToken) {
  wsOptions.headers = {
    'Authorization': `Bearer ${authToken}`
  };
}

const ws = new WebSocket(wsUrl, wsOptions);

// Setup readline for stdin
const rl = readline.createInterface({
  input: process.stdin,
  output: process.stdout,
  terminal: false
});

// Connection established
ws.on('open', () => {
  debug('WebSocket connection established');
});

// stdin → WebSocket
rl.on('line', (line) => {
  if (ws.readyState === WebSocket.OPEN) {
    debug('Sending to WebSocket:', line.substring(0, 100));
    ws.send(line);
  } else {
    console.error('WebSocket not ready, message dropped:', line.substring(0, 100));
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
  console.error('WebSocket error:', err.message);
  process.exit(1);
});

ws.on('close', (code, reason) => {
  debug('WebSocket connection closed:', code, reason.toString());
  console.error('WebSocket connection closed');
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
