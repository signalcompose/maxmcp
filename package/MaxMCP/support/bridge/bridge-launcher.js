#!/usr/bin/env node
/**
 * bridge-launcher.js
 * Node.js script for launching websocket-mcp-bridge via node.script
 */

const { spawn } = require('child_process');
const path = require('path');
const maxApi = require('max-api');

let bridgeProcess = null;
let wsUrl = 'ws://localhost:7400'; // Default

// Handle URL parameter from Max
maxApi.addHandler('url', (url) => {
  wsUrl = url;
  maxApi.post(`WebSocket URL set to: ${wsUrl}`);
});

// Handle start command
maxApi.addHandler('start', () => {
  if (bridgeProcess) {
    maxApi.post('Bridge already running');
    return;
  }

  const bridgeScript = path.join(__dirname, 'websocket-mcp-bridge.js');
  maxApi.post(`Starting bridge: ${bridgeScript} ${wsUrl}`);

  try {
    bridgeProcess = spawn('node', [bridgeScript, wsUrl], {
      cwd: __dirname,
      stdio: ['pipe', 'pipe', 'pipe'],
    });

    bridgeProcess.stdout.on('data', (data) => {
      const output = data.toString().trim();
      maxApi.post(`Bridge: ${output}`);
      maxApi.outlet(output);
    });

    bridgeProcess.stderr.on('data', (data) => {
      const error = data.toString().trim();
      maxApi.post(`Bridge error: ${error}`);
      maxApi.outlet(['error', error]);
    });

    bridgeProcess.on('close', (code) => {
      maxApi.post(`Bridge exited with code ${code}`);
      bridgeProcess = null;
    });

    maxApi.post('Bridge started successfully');
  } catch (error) {
    maxApi.post(`Failed to start bridge: ${error.message}`);
    maxApi.outlet(['error', error.message]);
  }
});

// Handle stop command
maxApi.addHandler('stop', () => {
  if (bridgeProcess) {
    bridgeProcess.kill();
    bridgeProcess = null;
    maxApi.post('Bridge stopped');
  } else {
    maxApi.post('Bridge not running');
  }
});

// Cleanup on exit
process.on('exit', () => {
  if (bridgeProcess) {
    bridgeProcess.kill();
  }
});

maxApi.post('Bridge launcher initialized');
