#!/bin/bash
#
# MaxMCP Package Installation Script
#
# Copies built binaries to package directory for distribution
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PACKAGE_DIR="$SCRIPT_DIR/package/MaxMCP"

echo "MaxMCP Package Installation"
echo "============================"
echo ""

# Check if externals exist
if [ ! -d "$HOME/externals/maxmcp.mxo" ]; then
    echo "Error: maxmcp.mxo not found in ~/externals/"
    echo "Please build the client first: cmake -B build -S . -DBUILD_MODE=client && cmake --build build"
    exit 1
fi

if [ ! -d "$HOME/externals/maxmcp.server.mxo" ]; then
    echo "Error: maxmcp.server.mxo not found in ~/externals/"
    echo "Please build the server first: cmake -B build -S . -DBUILD_MODE=server && cmake --build build"
    exit 1
fi

# Check if bridge exists
if [ ! -f "$SCRIPT_DIR/bridge/dist/maxmcp-bridge" ]; then
    echo "Error: maxmcp-bridge not found in bridge/dist/"
    echo "Please build the bridge first: cd bridge && npm run build"
    exit 1
fi

echo "Step 1: Copying externals..."
cp -R "$HOME/externals/maxmcp.mxo" "$PACKAGE_DIR/externals/"
cp -R "$HOME/externals/maxmcp.server.mxo" "$PACKAGE_DIR/externals/"
echo "  ✓ maxmcp.mxo copied"
echo "  ✓ maxmcp.server.mxo copied"
echo ""

echo "Step 2: Copying WebSocket bridge..."
cp "$SCRIPT_DIR/bridge/dist/maxmcp-bridge" "$PACKAGE_DIR/support/"
chmod +x "$PACKAGE_DIR/support/maxmcp-bridge"
echo "  ✓ maxmcp-bridge copied"
echo ""

echo "Step 3: Verifying package contents..."
ls -lh "$PACKAGE_DIR/externals/"
ls -lh "$PACKAGE_DIR/support/"
echo ""

echo "Installation complete!"
echo ""
echo "Package directory: $PACKAGE_DIR"
echo ""
echo "To install to Max:"
echo "  cp -R $PACKAGE_DIR ~/Documents/Max\ 9/Packages/"
echo ""
