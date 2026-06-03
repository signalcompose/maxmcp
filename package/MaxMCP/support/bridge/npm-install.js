#!/usr/bin/env node
/**
 * npm-install.js
 * Node.js script for installing MaxMCP bridge dependencies via node.script
 */

const { spawn } = require('child_process');
const maxApi = require('max-api');

// Get the bridge directory (same directory as this script)
const bridgeDir = __dirname;

maxApi.post('npm-install.js loaded');

// Handle bang message to start installation
maxApi.addHandler(maxApi.MESSAGE_TYPES.BANG, () => {
  maxApi.post(`Installing dependencies in: ${bridgeDir}`);

  // Install production dependencies only (runtime needs `ws`; eslint/jest/
  // prettier are devDependencies used for linting/testing and are unnecessary
  // in the end-user deploy environment). This also suppresses the deprecation
  // and dev-only audit warnings that the full install surfaces.
  const npmProcess = spawn('npm', ['install', '--omit=dev'], {
    cwd: bridgeDir,
    stdio: ['pipe', 'pipe', 'pipe'],
  });

  npmProcess.stdout.on('data', (data) => {
    const output = data.toString().trim();
    maxApi.post(`npm: ${output}`);
    maxApi.outlet(output);
  });

  npmProcess.stderr.on('data', (data) => {
    const error = data.toString().trim();
    maxApi.post(`npm stderr: ${error}`);
    maxApi.outlet(['error', error]);
  });

  npmProcess.on('close', (code) => {
    if (code === 0) {
      maxApi.post('npm install completed successfully');
      maxApi.outlet('Installation completed');
    } else {
      maxApi.post(`npm install exited with code ${code}`);
      maxApi.outlet(['error', `npm install failed with code ${code}`]);
    }
  });
});
