#!/bin/bash
# MaxMCP Build Script (macOS)

set -e  # Exit on error

echo "=== MaxMCP Build Script ==="

# Configuration
BUILD_TYPE="${1:-Debug}"  # Debug or Release
BUILD_DIR="build"

echo "Build Type: $BUILD_TYPE"

# Clean previous build (optional)
if [ "$2" == "clean" ]; then
    echo "Cleaning previous build..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"

# Configure CMake
echo "Configuring CMake..."
cmake -B "$BUILD_DIR" -S . \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

# Build
echo "Building..."
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE"

# Install to Max Library
echo "Installing to Max Library..."
INSTALL_DIR="$HOME/Documents/Max 9/Library"
mkdir -p "$INSTALL_DIR"
cp -v "$BUILD_DIR/MaxMCP.mxo" "$INSTALL_DIR/" || echo "Note: .mxo not yet generated (expected in early phase)"

echo "=== Build Complete ==="
echo "External: $INSTALL_DIR/MaxMCP.mxo"
