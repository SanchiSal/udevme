#!/bin/bash
# Build script for udevme
# Run from the project root directory

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"

echo "=== Building udevme ==="
echo ""

# Check for required tools
if ! command -v cmake &> /dev/null; then
    echo "ERROR: cmake not found"
    echo "Install with: sudo pacman -S cmake"
    exit 1
fi

if ! command -v make &> /dev/null; then
    echo "ERROR: make not found"
    echo "Install with: sudo pacman -S base-devel"
    exit 1
fi

# Check for Qt6
if ! pkg-config --exists Qt6Core 2>/dev/null; then
    echo "ERROR: Qt6 not found"
    echo "Install with: sudo pacman -S qt6-base"
    exit 1
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
echo "Configuring..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
echo ""
echo "Building..."
make -j$(nproc)

echo ""
echo "=== Build Complete ==="
echo ""
echo "Executable: $BUILD_DIR/udevme"
echo ""
echo "To install, run: ./packaging/install.sh"
echo ""
