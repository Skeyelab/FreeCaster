# FreeCaster - AirPlay VST3 Plugin

A cross-platform VST3 plugin that streams real-time audio from your DAW to AirPlay-enabled speakers and devices, similar to Audiomovers AirCaster.

## Features

- ✅ Real-time audio streaming to AirPlay devices
- ✅ Automatic device discovery via mDNS/Bonjour
- ✅ Cross-platform support (macOS, Windows, Linux)
- ✅ Low-latency buffering
- ✅ Simple, intuitive GUI
- ✅ VST3 and Standalone formats

## Platform Support

### macOS
- Native AirPlay integration via AVFoundation
- Best audio quality and device compatibility
- Supports AirPlay and AirPlay 2 devices

### Windows
- RAOP protocol implementation
- Compatible with most AirPlay devices
- Requires network with mDNS support

### Linux
- RAOP protocol implementation
- Uses Avahi for device discovery
- Full compatibility with AirPlay devices

## Use Cases

- **Mix Checking**: Preview your mixes on different speakers without bouncing
- **Remote Monitoring**: Listen in another room while working
- **Consumer Testing**: Hear how your mix sounds on consumer-grade speakers
- **Multi-Room**: Stream different channels to different rooms
- **Client Playback**: Send audio to client listening areas

## Quick Start

1. Build the plugin (see instructions below)
2. Install the VST3 to your plugins folder
3. Load it in your DAW on your master track
4. Select an AirPlay device from the list
5. Click "Connect" and start playing

For detailed setup instructions, see [USER_GUIDE.md](USER_GUIDE.md).

## Building

### Requirements

- CMake 3.15+
- C++17 compiler
- JUCE (automatically downloaded)
- OpenSSL 3.0 or later (for RAOP authentication)

### Quick Build

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### Platform-Specific Notes

**macOS:**
- Uses native AVFoundation for AirPlay
- Automatically detected and configured
- Requires macOS 10.13+

**Windows/Linux:**
- RAOP protocol implementation
- Requires OpenSSL for authentication
- Ensure mDNS service is running (or Avahi on Linux)

## Testing

The project includes comprehensive automated testing capabilities:

```bash
# Run automated AirPlay device discovery and connection testing
python3 automated_airplay_test.py
```

This script will:
- Discover all AirPlay devices on your network
- Attempt to connect to each device
- Log detailed RTSP handshake information
- Provide connection status summary

For more information on testing, see [CURRENT_STATUS.md](CURRENT_STATUS.md).

## Architecture

The plugin uses a modular architecture with platform-specific implementations:

```
┌─────────────────────────────────────┐
│         VST3 Plugin (JUCE)          │
├─────────────────────────────────────┤
│        Plugin Processor             │
│      (Audio Processing)             │
├─────────────────────────────────────┤
│       AirPlay Manager               │
│    (Streaming Coordination)         │
├──────────┬──────────────────────────┤
│ macOS:   │ Windows/Linux:           │
│ Native   │ RAOP Protocol            │
│ AirPlay  │ Implementation           │
└──────────┴──────────────────────────┘
       │              │
       └──────┬───────┘
              │
       AirPlay Devices
```

## Components

- **PluginProcessor**: Main audio processing and VST3 interface
- **PluginEditor**: GUI with device list and connection controls
- **AirPlayManager**: Coordinates streaming and platform selection
- **DeviceDiscovery**: mDNS device discovery across platforms
- **AudioEncoder**: PCM/ALAC encoding for AirPlay
- **StreamBuffer**: Thread-safe circular buffer for audio data
- **Platform Implementations**:
  - **AirPlayMac**: macOS native implementation
  - **AirPlayWindows**: Windows RAOP client
  - **AirPlayLinux**: Linux RAOP client

## Technical Details

- **Audio Format**: Stereo PCM 16-bit
- **Sample Rates**: 44.1 kHz, 48 kHz
- **Latency**: 200-500ms (network dependent)
- **Protocol**: RAOP (Remote Audio Output Protocol)
- **Discovery**: mDNS/Bonjour

## Known Limitations

- Stereo only (no multichannel/surround)
- Network latency (~200-500ms) makes it unsuitable for tracking
- Some AirPlay 2 advanced features not implemented
- Authentication-required devices may not work
- ALAC encoding not yet fully implemented (uses PCM)

## Future Enhancements

- [ ] ALAC encoding for better efficiency
- [ ] Multi-device simultaneous streaming
- [ ] Authentication support
- [ ] Sample rate conversion
- [ ] Latency compensation
- [ ] Network quality monitoring
- [ ] Preset management
- [ ] AAX format support

## Troubleshooting

### No devices found
- Ensure computer and devices are on same network
- Check firewall settings
- Verify mDNS/Bonjour is enabled

### Connection failures
- Restart AirPlay device
- Check device compatibility
- Disable VPN if active

### Audio dropouts
- Increase DAW buffer size
- Use wired Ethernet connection
- Check network bandwidth

See [USER_GUIDE.md](USER_GUIDE.md) for more troubleshooting tips.

## License

This project is provided as-is for educational and personal use.

## Acknowledgments

- Built with [JUCE](https://juce.com/)
- Inspired by Audiomovers AirCaster (FreeCaster is a free, open-source alternative)
- RAOP protocol information from various open-source projects

## Contributing

Contributions welcome! Areas needing work:
- ALAC encoder implementation
- Enhanced device authentication
- Improved cross-platform mDNS discovery
- Additional audio format support
- Better error handling

## Disclaimer

This is an educational/reference implementation. AirPlay is a trademark of Apple Inc. This project is not affiliated with or endorsed by Apple Inc.
