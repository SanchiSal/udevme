#!/bin/bash
# Uninstall script for udevme

set -e

INSTALL_DIR="$HOME/.local/bin/udevme"
BIN_LINK="$HOME/.local/bin/udevme-run"
DESKTOP_FILE="$HOME/.local/share/applications/udevme.desktop"
SYSTEM_RULES="/etc/udev/rules.d/99-udevme.rules"

echo "=== udevme Uninstaller ==="
echo ""

# Remove symlink if exists
if [ -L "$BIN_LINK" ] || [ -f "$BIN_LINK" ]; then
    rm -f "$BIN_LINK"
    echo "Removed: $BIN_LINK"
fi

# Remove install directory
if [ -d "$INSTALL_DIR" ]; then
    rm -rf "$INSTALL_DIR"
    echo "Removed: $INSTALL_DIR"
else
    echo "Install directory not found (already removed?)"
fi

# Remove desktop entry
if [ -f "$DESKTOP_FILE" ]; then
    rm -f "$DESKTOP_FILE"
    echo "Removed: $DESKTOP_FILE"
else
    echo "Desktop entry not found (already removed?)"
fi

# Update desktop database
if command -v update-desktop-database &> /dev/null; then
    update-desktop-database "$HOME/.local/share/applications" 2>/dev/null || true
fi

# Optionally remove system rules
if [ -f "$SYSTEM_RULES" ]; then
    echo ""
    echo "System rules file found: $SYSTEM_RULES"
    read -p "Remove system udev rules? [y/N] " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        sudo rm -f "$SYSTEM_RULES"
        echo "Removed: $SYSTEM_RULES"
        echo "Reloading udev rules..."
        sudo udevadm control --reload-rules
        sudo udevadm trigger
        echo "udev rules reloaded"
    else
        echo "System rules kept"
    fi
fi

echo ""
echo "=== Uninstall Complete ==="
echo ""
