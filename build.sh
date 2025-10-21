#!/bin/bash
# FreeCaster Build Script

set -e

echo "===================================="
echo "FreeCaster Build Script"
echo "===================================="
echo ""

# Detect platform
if [[ "$OSTYPE" == "darwin"* ]]; then
    PLATFORM="macOS"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    PLATFORM="Linux"
else
    echo "Unsupported platform: $OSTYPE"
    exit 1
fi

echo "Platform: $PLATFORM"

# Build configuration
CONFIG=${1:-Release}
echo "Build configuration: $CONFIG"
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
cmake ..

# Build
echo ""
echo "Building plugin..."
cmake --build . --config $CONFIG

echo ""
echo "===================================="
echo "Build complete!"
echo "===================================="
echo ""
echo "Plugin location:"

if [[ "$PLATFORM" == "macOS" ]]; then
    echo "  VST3: FreeCaster_artefacts/$CONFIG/VST3/FreeCaster.vst3"
    echo "  Standalone: FreeCaster_artefacts/$CONFIG/Standalone/FreeCaster.app"
    echo ""
    echo "VST3 automatically installed to:"
    echo "  ~/Library/Audio/Plug-Ins/VST3/FreeCaster.vst3"
else
    echo "  VST3: FreeCaster_artefacts/$CONFIG/VST3/FreeCaster.vst3"
    echo "  Standalone: FreeCaster_artefacts/$CONFIG/Standalone/FreeCaster"
fi

echo ""
echo "To test the standalone version:"
if [[ "$PLATFORM" == "macOS" ]]; then
    echo "  ./FreeCaster_artefacts/$CONFIG/Standalone/FreeCaster.app/Contents/MacOS/FreeCaster"
else
    echo "  ./FreeCaster_artefacts/$CONFIG/Standalone/FreeCaster"
fi
