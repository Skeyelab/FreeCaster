#!/bin/bash
# FreeCaster Build Script (macOS only)

set -e

echo "===================================="
echo "FreeCaster Build Script"
echo "===================================="
echo ""

# Verify we're on macOS
if [[ "$OSTYPE" != "darwin"* ]]; then
    echo "ERROR: This project is macOS-only"
    echo "Unsupported platform: $OSTYPE"
    exit 1
fi

echo "Platform: macOS"

# Build configuration
CONFIG=${1:-Release}
echo "Build configuration: $CONFIG"
echo ""

# Detect architecture
ARCH=$(uname -m)
echo "Architecture: $ARCH"
echo ""

# Create build directory
if [ -d "build" ]; then
    echo "Build directory exists. Cleaning..."
    rm -rf build
fi

mkdir build
cd build

# Run CMake
echo "Running CMake..."
cmake -DCMAKE_BUILD_TYPE=$CONFIG ..

# Build
echo ""
echo "Building plugin..."
cmake --build . --config $CONFIG -j$(sysctl -n hw.ncpu)

echo ""
echo "===================================="
echo "Build complete!"
echo "===================================="
echo ""
echo "Plugin locations:"
echo "  VST3: build/FreeCaster_artefacts/VST3/FreeCaster.vst3"
echo "  AU: build/FreeCaster_artefacts/AU/FreeCaster.component"
echo "  Standalone: build/FreeCaster_artefacts/Standalone/FreeCaster.app"
echo ""
echo "VST3 and AU automatically installed to:"
echo "  ~/Library/Audio/Plug-Ins/VST3/"
echo "  ~/Library/Audio/Plug-Ins/Components/"
echo ""
echo "To run the standalone version:"
echo "  open build/FreeCaster_artefacts/Standalone/FreeCaster.app"
echo ""
echo "Or from command line:"
echo "  ./build/FreeCaster_artefacts/Standalone/FreeCaster.app/Contents/MacOS/FreeCaster"
