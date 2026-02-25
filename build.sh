#!/bin/bash
#
# MaxMCP Build Script
#
# Phases: Configure → Build → Test → Install (to package/MaxMCP)
#
# Usage:
#   ./build.sh              # Debug build (no tests)
#   ./build.sh --test       # Debug build with tests
#   ./build.sh Release      # Release build
#   ./build.sh Release --test
#   ./build.sh --clean      # Clean build directory first
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
PACKAGE_DIR="$SCRIPT_DIR/package/MaxMCP"

# Parse arguments
BUILD_TYPE="Debug"
RUN_TESTS=false
CLEAN=false

for arg in "$@"; do
    case "$arg" in
        --test)   RUN_TESTS=true ;;
        --clean)  CLEAN=true ;;
        Debug|Release) BUILD_TYPE="$arg" ;;
        *)
            echo "Unknown argument: $arg"
            echo "Usage: $0 [Debug|Release] [--test] [--clean]"
            exit 1
            ;;
    esac
done

echo "=== MaxMCP Build ==="
echo "  Type:  $BUILD_TYPE"
echo "  Tests: $RUN_TESTS"
echo ""

# Clean
if [ "$CLEAN" = true ]; then
    echo "[1/4] Cleaning build directory..."
    rm -rf "$BUILD_DIR"
else
    echo "[1/4] Clean: skipped (use --clean to enable)"
fi

# Configure
echo "[2/4] Configuring..."
CMAKE_ARGS="-B $BUILD_DIR -S $SCRIPT_DIR -DCMAKE_BUILD_TYPE=$BUILD_TYPE"
if [ "$RUN_TESTS" = true ]; then
    CMAKE_ARGS="$CMAKE_ARGS -DBUILD_TESTS=ON"
fi
cmake $CMAKE_ARGS

# Build
echo "[3/4] Building..."
cmake --build "$BUILD_DIR"

# Test
if [ "$RUN_TESTS" = true ]; then
    echo "[4/4] Running tests..."
    cd "$BUILD_DIR" && ctest --output-on-failure && cd "$SCRIPT_DIR"
else
    echo "[4/4] Tests: skipped (use --test to enable)"
fi

# Install to package directory
echo ""
echo "Installing to $PACKAGE_DIR ..."
cmake --install "$BUILD_DIR" --prefix "$PACKAGE_DIR"

echo ""
echo "=== Build Complete ==="
echo "Package ready at: $PACKAGE_DIR"
echo ""
echo "Next: run ./deploy.sh to deploy to Max 9"
