#!/bin/bash
# Install script for udevme
# Run from project root: ./packaging/install.sh

set -e

# Get project root (parent of packaging folder)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"

INSTALL_DIR="$HOME/.local/bin/udevme"
DESKTOP_DIR="$HOME/.local/share/applications"

echo "=== udevme Installer ==="
echo ""
echo "Project: $PROJECT_DIR"
echo "Install to: $INSTALL_DIR"
echo ""

# Check if build exists
if [ ! -f "$BUILD_DIR/udevme" ]; then
    echo "ERROR: Build not found at $BUILD_DIR/udevme"
    echo "Please run ./packaging/build.sh first"
    exit 1
fi

# Remove existing install if present (handles root-owned files issue)
if [ -d "$INSTALL_DIR" ]; then
    echo "Removing previous installation..."
    rm -rf "$INSTALL_DIR" 2>/dev/null || sudo rm -rf "$INSTALL_DIR"
fi

# Create directories
mkdir -p "$INSTALL_DIR"
mkdir -p "$DESKTOP_DIR"

# Copy executable
cp "$BUILD_DIR/udevme" "$INSTALL_DIR/"
chmod +x "$INSTALL_DIR/udevme"
echo "Installed executable"

# Copy icon
if [ -f "$PROJECT_DIR/resources/udevme_icon.png" ]; then
    cp "$PROJECT_DIR/resources/udevme_icon.png" "$INSTALL_DIR/"
    echo "Installed icon"
else
    echo "Warning: Icon not found"
fi

# Create desktop entry
cat > "$DESKTOP_DIR/udevme.desktop" << EOF
[Desktop Entry]
Type=Application
Name=udevme
GenericName=udev Rule Manager
Comment=Manage udev rules for WebHID and hidraw device access
Exec=$INSTALL_DIR/udevme
Icon=$INSTALL_DIR/udevme_icon.png
Terminal=false
Categories=System;Settings;HardwareSettings;
Keywords=udev;usb;hid;webhid;hidraw;device;permissions;
StartupNotify=true
EOF
echo "Created desktop entry"

# Update desktop database
if command -v update-desktop-database &> /dev/null; then
    update-desktop-database "$DESKTOP_DIR" 2>/dev/null || true
fi

echo ""
echo "=== Installation Complete ==="
echo ""
echo "Run: $INSTALL_DIR/udevme"
echo ""
