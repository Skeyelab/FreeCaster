#!/bin/bash
# FreeCaster Release Script (macOS only)
# Builds optimized release, signs code, and packages for distribution

set -e

echo "===================================="
echo "FreeCaster Release Builder"
echo "===================================="
echo ""

# Verify we're on macOS
if [[ "$OSTYPE" != "darwin"* ]]; then
    echo "ERROR: This project is macOS-only"
    echo "Unsupported platform: $OSTYPE"
    exit 1
fi

# Get version from user or use default
VERSION=${1:-1.0.0}
echo "Release version: $VERSION"

# Get architecture
ARCH=$(uname -m)
echo "Architecture: $ARCH"
echo ""

# Clean and build
echo "Building Release configuration..."
if [ -d "build" ]; then
    rm -rf build
fi

mkdir build
cd build

cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release -j$(sysctl -n hw.ncpu)

echo ""
echo "===================================="
echo "Build Complete!"
echo "===================================="
echo ""

# Navigate to artifacts
cd FreeCaster_artefacts/Release

# Create release directory
RELEASE_DIR="../../release/FreeCaster-${VERSION}-${ARCH}"
mkdir -p "$RELEASE_DIR"

echo "Packaging for release..."
echo ""

# Copy VST3 plugin
if [ -d "VST3/FreeCaster.vst3" ]; then
    echo "Packaging VST3 plugin..."
    cp -r VST3/FreeCaster.vst3 "$RELEASE_DIR/"
fi

# Copy AU plugin
if [ -d "AU/FreeCaster.component" ]; then
    echo "Packaging AU plugin..."
    cp -r AU/FreeCaster.component "$RELEASE_DIR/"
fi

# Copy Standalone app
if [ -d "Standalone/FreeCaster.app" ]; then
    echo "Packaging Standalone app..."
    cp -r Standalone/FreeCaster.app "$RELEASE_DIR/"
fi

# Create README
cat > "$RELEASE_DIR/README.txt" << 'EOF'
FreeCaster - AirPlay VST3 Plugin for macOS

Installation:
1. VST3 Plugin: Copy FreeCaster.vst3 to ~/Library/Audio/Plug-Ins/VST3/
2. AU Plugin: Copy FreeCaster.component to ~/Library/Audio/Plug-Ins/Components/
3. Standalone: Double-click FreeCaster.app to run

For detailed documentation, visit:
https://github.com/Skeyelab/FreeCaster

Requirements:
- macOS 10.13 or later
- Intel or Apple Silicon processor

Features:
- Real-time audio streaming to AirPlay devices
- Automatic device discovery
- Low-latency streaming
- Support for multiple AirPlay devices

Troubleshooting:
- Ensure all AirPlay devices are on the same network
- Check System Preferences > Sound for connected devices
- Restart the app if device discovery issues occur
EOF

echo ""
echo "===================================="
echo "Release packaging complete!"
echo "===================================="
echo ""
echo "Release location:"
echo "  $RELEASE_DIR"
echo ""
echo "Contents:"
ls -la "$RELEASE_DIR"
echo ""
echo "Next steps:"
echo "1. Test the plugins in your DAW"
echo "2. Create a GitHub release at:"
echo "   https://github.com/Skeyelab/FreeCaster/releases/new"
echo "3. Upload the plugins as release assets"
echo "4. Include the changelog in release notes"
echo ""
