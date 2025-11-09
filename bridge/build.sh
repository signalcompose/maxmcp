#!/bin/bash
#
# MaxMCP Bridge Build Script
#
# Builds a single executable binary containing:
# - Node.js runtime
# - websocket-mcp-bridge.js
# - node_modules/ws
#
# Output: dist/maxmcp-bridge (~40MB)

set -e

echo "🔨 Building MaxMCP WebSocket Bridge..."

# Check if npm is available
if ! command -v npm &> /dev/null; then
    echo "❌ npm not found. Please install Node.js."
    exit 1
fi

# Install dependencies
echo "📦 Installing dependencies..."
npm install

# Install pkg globally if not available
if ! command -v pkg &> /dev/null; then
    echo "📦 Installing pkg globally..."
    npm install -g pkg
fi

# Build binary
echo "🏗️  Building binary with pkg..."
pkg . --targets node18-macos-arm64 --output dist/maxmcp-bridge

# Check output
if [ -f "dist/maxmcp-bridge" ]; then
    FILE_SIZE=$(du -h dist/maxmcp-bridge | awk '{print $1}')
    echo "✅ Build successful!"
    echo "📄 Binary: dist/maxmcp-bridge (${FILE_SIZE})"
    echo ""
    echo "Usage:"
    echo "  ./dist/maxmcp-bridge ws://localhost:7400"
    echo "  ./dist/maxmcp-bridge wss://remote:7400 auth-token-123"
else
    echo "❌ Build failed"
    exit 1
fi
