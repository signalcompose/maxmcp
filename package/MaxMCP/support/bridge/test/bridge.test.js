/**
 * MaxMCP Bridge Unit Tests
 *
 * Tests for websocket-mcp-bridge.js
 */

const WebSocket = require('ws');
const { spawn } = require('child_process');
const path = require('path');

describe('maxmcp-bridge', () => {
  let wss;
  const TEST_PORT = 7401;
  const TEST_URL = `ws://localhost:${TEST_PORT}`;

  beforeAll(() => {
    // Setup test WebSocket server
    wss = new WebSocket.Server({ port: TEST_PORT });
  });

  afterAll(() => {
    // Cleanup
    if (wss) {
      wss.close();
    }
  });

  describe('WebSocket connection', () => {
    test('should connect to WebSocket server', (done) => {
      wss.once('connection', (ws) => {
        expect(ws.readyState).toBe(WebSocket.OPEN);
        ws.close();
        done();
      });

      // Spawn bridge process
      const bridge = spawn('node', [path.join(__dirname, '../websocket-mcp-bridge.js'), TEST_URL]);

      // Cleanup
      setTimeout(() => {
        bridge.kill();
      }, 1000);
    });

    test('should handle authentication header', (done) => {
      const testToken = 'test-token-123';

      wss.once('connection', (ws, req) => {
        const authHeader = req.headers['authorization'];
        expect(authHeader).toBe(`Bearer ${testToken}`);
        ws.close();
        done();
      });

      const bridge = spawn('node', [
        path.join(__dirname, '../websocket-mcp-bridge.js'),
        TEST_URL,
        testToken,
      ]);

      setTimeout(() => {
        bridge.kill();
      }, 1000);
    });
  });

  describe('Message translation', () => {
    test('should forward stdin to WebSocket', (done) => {
      const testMessage = '{"jsonrpc":"2.0","method":"test","id":1}';

      wss.once('connection', (ws) => {
        ws.once('message', (data) => {
          expect(data.toString()).toBe(testMessage);
          ws.close();
          done();
        });
      });

      const bridge = spawn('node', [path.join(__dirname, '../websocket-mcp-bridge.js'), TEST_URL]);

      // Wait for connection, then send message
      setTimeout(() => {
        bridge.stdin.write(testMessage + '\n');
      }, 500);

      setTimeout(() => {
        bridge.kill();
      }, 2000);
    });

    test('should forward WebSocket to stdout', (done) => {
      const testResponse = '{"jsonrpc":"2.0","result":"ok","id":1}';
      let output = '';

      const bridge = spawn('node', [path.join(__dirname, '../websocket-mcp-bridge.js'), TEST_URL]);

      bridge.stdout.on('data', (data) => {
        output += data.toString();
        if (output.includes(testResponse)) {
          expect(output).toContain(testResponse);
          bridge.kill();
          done();
        }
      });

      wss.once('connection', (ws) => {
        // Send response after connection
        setTimeout(() => {
          ws.send(testResponse);
        }, 100);
      });

      setTimeout(() => {
        if (!done.mock) {
          bridge.kill();
        }
      }, 2000);
    });
  });

  describe('Error handling', () => {
    test('should exit on connection error', (done) => {
      const invalidUrl = 'ws://localhost:9999'; // No server

      const bridge = spawn('node', [
        path.join(__dirname, '../websocket-mcp-bridge.js'),
        invalidUrl,
      ]);

      bridge.on('exit', (code) => {
        expect(code).toBe(1); // Error exit code
        done();
      });

      setTimeout(() => {
        bridge.kill();
        done();
      }, 2000);
    });

    test('should exit gracefully on SIGINT', (done) => {
      const bridge = spawn('node', [path.join(__dirname, '../websocket-mcp-bridge.js'), TEST_URL]);

      setTimeout(() => {
        bridge.kill('SIGINT');
      }, 500);

      bridge.on('exit', (code) => {
        expect(code).toBe(0); // Graceful exit
        done();
      });
    });
  });
});
