#!/bin/bash

# Debug script for AkisVG
echo "=== Debug Run ==="
echo "Working directory: $(pwd)"
echo "LD_LIBRARY_PATH: $LD_LIBRARY_PATH"
echo "QT_QPA_PLATFORM: $QT_QPA_PLATFORM"

# List available Qt platform plugins
echo "=== Qt Platform Plugins ==="
find /usr/lib/qt6/plugins/platforms/ -name "*.so" -exec basename {} \; 2>/dev/null
find /usr/lib/qt/plugins/platforms/ -name "*.so" -exec basename {} \; 2>/dev/null

# Run with Qt debugging
export QT_LOGGING_RULES="qt.qpa.*=debug"
export QT_DEBUG_PLUGINS=1

# Try with different platform plugins
for platform in xcb wayland; do
    echo "=== Trying with QT_QPA_PLATFORM=$platform ==="
    QT_QPA_PLATFORM=$platform gdb -ex "run" -ex "bt" -ex "quit" --args ./AkisVG
done
