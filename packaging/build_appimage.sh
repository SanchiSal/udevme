#!/bin/bash
# Build udevme AppImage
# Requires: linuxdeploy, linuxdeploy-plugin-qt, appimagetool

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"
APPDIR="$BUILD_DIR/AppDir"

echo "=== Building udevme AppImage ==="
echo "Project directory: $PROJECT_DIR"

# Check for required tools
check_tool() {
    if ! command -v "$1" &> /dev/null; then
        echo "ERROR: $1 not found. Please install it first."
        echo ""
        echo "Installation instructions:"
        case "$1" in
            linuxdeploy)
                echo "  wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
                echo "  chmod +x linuxdeploy-x86_64.AppImage"
                echo "  sudo mv linuxdeploy-x86_64.AppImage /usr/local/bin/linuxdeploy"
                ;;
            linuxdeploy-plugin-qt)
                echo "  wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"
                echo "  chmod +x linuxdeploy-plugin-qt-x86_64.AppImage"
                echo "  sudo mv linuxdeploy-plugin-qt-x86_64.AppImage /usr/local/bin/linuxdeploy-plugin-qt"
                ;;
            appimagetool)
                echo "  wget https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage"
                echo "  chmod +x appimagetool-x86_64.AppImage"
                echo "  sudo mv appimagetool-x86_64.AppImage /usr/local/bin/appimagetool"
                ;;
        esac
        exit 1
    fi
}

# Build the project first
echo ""
echo "=== Building project ==="
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
make -j$(nproc)

# Create AppDir structure
echo ""
echo "=== Creating AppDir ==="
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin"
mkdir -p "$APPDIR/usr/share/applications"
mkdir -p "$APPDIR/usr/share/icons/hicolor/256x256/apps"

# Copy binary
cp "$BUILD_DIR/udevme" "$APPDIR/usr/bin/"

# Copy desktop file
cp "$PROJECT_DIR/packaging/udevme.desktop" "$APPDIR/usr/share/applications/"

# Copy icon
cp "$PROJECT_DIR/resources/udevme_icon.png" "$APPDIR/usr/share/icons/hicolor/256x256/apps/udevme.png"

# Create AppRun (simple version)
cat > "$APPDIR/AppRun" << 'EOF'
#!/bin/bash
SELF=$(readlink -f "$0")
HERE=${SELF%/*}
export PATH="${HERE}/usr/bin:${PATH}"
export LD_LIBRARY_PATH="${HERE}/usr/lib:${LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="${HERE}/usr/plugins"
export QT_QPA_PLATFORM_PLUGIN_PATH="${HERE}/usr/plugins/platforms"
exec "${HERE}/usr/bin/udevme" "$@"
EOF
chmod +x "$APPDIR/AppRun"

# Link icon and desktop file to AppDir root
ln -sf usr/share/icons/hicolor/256x256/apps/udevme.png "$APPDIR/udevme.png"
ln -sf usr/share/applications/udevme.desktop "$APPDIR/udevme.desktop"

# Use linuxdeploy if available, otherwise manual bundling
if command -v linuxdeploy &> /dev/null; then
    echo ""
    echo "=== Running linuxdeploy ==="
    
    export QMAKE=$(which qmake6 || which qmake)
    
    linuxdeploy --appdir "$APPDIR" \
        --plugin qt \
        --output appimage \
        --desktop-file "$APPDIR/usr/share/applications/udevme.desktop" \
        --icon-file "$APPDIR/usr/share/icons/hicolor/256x256/apps/udevme.png"
    
    # Move AppImage to project root
    mv udevme*.AppImage "$PROJECT_DIR/" 2>/dev/null || true
else
    echo ""
    echo "=== Manual Qt library bundling ==="
    
    # Get Qt library path
    QT_LIB_PATH=$(qmake6 -query QT_INSTALL_LIBS 2>/dev/null || qmake -query QT_INSTALL_LIBS)
    QT_PLUGIN_PATH=$(qmake6 -query QT_INSTALL_PLUGINS 2>/dev/null || qmake -query QT_INSTALL_PLUGINS)
    
    mkdir -p "$APPDIR/usr/lib"
    mkdir -p "$APPDIR/usr/plugins"
    
    # Copy required Qt libraries
    for lib in Core Gui Widgets Concurrent; do
        cp -L "$QT_LIB_PATH/libQt6${lib}.so"* "$APPDIR/usr/lib/" 2>/dev/null || \
        cp -L "$QT_LIB_PATH/libQt5${lib}.so"* "$APPDIR/usr/lib/" 2>/dev/null || true
    done
    
    # Copy xcb platform plugin
    mkdir -p "$APPDIR/usr/plugins/platforms"
    cp -r "$QT_PLUGIN_PATH/platforms/libqxcb.so" "$APPDIR/usr/plugins/platforms/" 2>/dev/null || true
    
    # Copy wayland platform if available
    cp -r "$QT_PLUGIN_PATH/platforms/libqwayland"*.so "$APPDIR/usr/plugins/platforms/" 2>/dev/null || true
    
    # Use appimagetool if available
    if command -v appimagetool &> /dev/null; then
        echo ""
        echo "=== Creating AppImage with appimagetool ==="
        cd "$BUILD_DIR"
        ARCH=x86_64 appimagetool "$APPDIR" "$PROJECT_DIR/udevme-x86_64.AppImage"
    else
        echo ""
        echo "WARNING: appimagetool not found. AppDir created but AppImage not generated."
        echo "AppDir location: $APPDIR"
        echo ""
        echo "To create AppImage manually, install appimagetool and run:"
        echo "  ARCH=x86_64 appimagetool $APPDIR udevme-x86_64.AppImage"
    fi
fi

echo ""
echo "=== Done ==="
echo "AppDir: $APPDIR"
ls -la "$PROJECT_DIR/"*.AppImage 2>/dev/null || echo "AppImage file not found (see above for manual steps)"
