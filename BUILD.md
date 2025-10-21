# Building FreeCaster

## Prerequisites

### All Platforms
- CMake 3.15 or higher
- C++17 compatible compiler
- Git (for fetching JUCE)

### macOS
- Xcode 12+
- macOS 10.13+

### Windows
- Visual Studio 2019 or 2022
- Windows 10+

### Linux
- GCC 9+ or Clang 10+
- ALSA development libraries
- Avahi development libraries

## Build Instructions

### macOS

```bash
cd AirPlayPlugin
mkdir build && cd build
cmake .. -G Xcode
cmake --build . --config Release
```

The plugin will be built to:
- VST3: `build/AirPlayPlugin_artefacts/Release/VST3/FreeCaster.vst3`

### Windows

```bash
cd AirPlayPlugin
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```

The plugin will be built to:
- VST3: `build\AirPlayPlugin_artefacts\Release\VST3\FreeCaster.vst3`

### Linux

```bash
cd AirPlayPlugin
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

Install dependencies on Ubuntu/Debian:
```bash
sudo apt-get install libasound2-dev libavahi-client-dev libavahi-common-dev
```

## Installation

### macOS
Copy the VST3 bundle to:
- `~/Library/Audio/Plug-Ins/VST3/`
- `/Library/Audio/Plug-Ins/VST3/` (system-wide)

### Windows
Copy the VST3 folder to:
- `C:\Program Files\Common Files\VST3\`

### Linux
Copy the VST3 file to:
- `~/.vst3/`
- `/usr/local/lib/vst3/` (system-wide)

## Testing

The plugin includes a Standalone version for testing without a DAW.

Run the standalone:
```bash
# macOS
./build/AirPlayPlugin_artefacts/Release/Standalone/FreeCaster.app/Contents/MacOS/FreeCaster

# Windows
.\build\AirPlayPlugin_artefacts\Release\Standalone\FreeCaster.exe

# Linux
./build/AirPlayPlugin_artefacts/Release/Standalone/FreeCaster
```
