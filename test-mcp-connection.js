#!/usr/bin/env node
/**
 * Test script for MCP connection via bridge
 */

const { spawn } = require('child_process');
const path = require('path');

const bridgePath = path.join(process.env.HOME, 'Documents/Max 9/Packages/MaxMCP/support/maxmcp-mcp-bridge.js');

console.log('Starting MCP bridge test...');

// Spawn bridge process
const bridge = spawn(bridgePath, ['--debug'], {
    stdio: ['pipe', 'pipe', 'pipe']
});

let receivedResponse = false;

// Handle stdout (responses from Max)
bridge.stdout.on('data', (data) => {
    const message = data.toString();
    console.log('Response from Max:', message);
    receivedResponse = true;

    // Close after receiving response
    setTimeout(() => {
        bridge.stdin.end();
        process.exit(0);
    }, 1000);
});

// Handle stderr (debug logs)
bridge.stderr.on('data', (data) => {
    console.log('Bridge log:', data.toString());
});

// Handle process exit
bridge.on('close', (code) => {
    console.log(`Bridge process exited with code ${code}`);
    if (!receivedResponse) {
        console.log('ERROR: No response received from Max');
        process.exit(1);
    }
});

// Wait for connection, then send initialize request
setTimeout(() => {
    console.log('Sending initialize request...');
    const initRequest = {
        jsonrpc: '2.0',
        method: 'initialize',
        params: {
            protocolVersion: '2024-11-05',
            capabilities: {},
            clientInfo: {
                name: 'test',
                version: '1.0.0'
            }
        },
        id: 1
    };

    bridge.stdin.write(JSON.stringify(initRequest) + '\n');
}, 2000);

// Timeout after 10 seconds
setTimeout(() => {
    if (!receivedResponse) {
        console.log('TIMEOUT: No response after 10 seconds');
        bridge.kill();
        process.exit(1);
    }
}, 10000);
