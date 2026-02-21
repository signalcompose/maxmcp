#!/bin/bash
#
# MaxMCP Deploy Script
#
# Phases: Remove old package → Deploy to Max 9 Packages
#
# Usage:
#   ./deploy.sh             # Deploy package/MaxMCP to Max 9 Packages
#
# Prerequisites:
#   Run ./build.sh first to build and install to package/MaxMCP
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PACKAGE_DIR="$SCRIPT_DIR/package/MaxMCP"
MAX_PACKAGES="$HOME/Documents/Max 9/Packages"
DEST="$MAX_PACKAGES/MaxMCP"

echo "=== MaxMCP Deploy ==="
echo ""

# Verify source exists
if [ ! -d "$PACKAGE_DIR" ]; then
    echo "Error: $PACKAGE_DIR not found."
    echo "Run ./build.sh first."
    exit 1
fi

# Verify externals exist
if [ ! -d "$PACKAGE_DIR/externals/maxmcp.mxo" ]; then
    echo "Error: maxmcp.mxo not found in $PACKAGE_DIR/externals/"
    echo "Run ./build.sh first."
    exit 1
fi

# Remove old package
if [ -d "$DEST" ]; then
    echo "[1/2] Removing old package..."
    rm -rf "$DEST"
    echo "  Removed: $DEST"
else
    echo "[1/2] No old package found (clean install)"
fi

# Deploy
echo "[2/2] Deploying..."
cp -R "$PACKAGE_DIR" "$DEST"
echo "  Deployed: $DEST"

echo ""
echo "=== Deploy Complete ==="
echo ""
echo "Next steps:"
echo "  1. Restart Max to load the updated external"
echo "  2. If using Claude Code MCP, restart Claude Code too"
