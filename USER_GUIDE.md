# FreeCaster - User Guide

## Overview

FreeCaster allows you to send audio from your DAW directly to AirPlay-enabled speakers and devices in real-time, without needing to bounce or export your mix.

## Features

- Real-time audio streaming to AirPlay devices
- Automatic device discovery on your local network
- Low-latency buffering for minimal delay
- Cross-platform support (macOS, Windows, Linux)
- Simple, intuitive interface

## Getting Started

### 1. Network Setup

Ensure that:
- Your computer and AirPlay devices are on the same network
- Firewall settings allow mDNS/Bonjour traffic
- AirPlay devices are powered on and discoverable

### 2. Loading the Plugin

1. Open your DAW
2. Load "FreeCaster" as an insert effect on your master track
3. Or load it on any track you want to monitor separately

### 3. Connecting to a Device

1. The plugin will automatically scan for AirPlay devices
2. Available devices will appear in the device list
3. Select a device from the list
4. Click "Connect"
5. The status will show "Connected to: [Device Name]"

### 4. Streaming Audio

Once connected, all audio passing through the plugin will stream to the selected AirPlay device:

- Play your DAW project normally
- Audio will stream in real-time
- Use this to:
  - Check mixes on different speakers
  - Monitor in another room
  - Preview on consumer-grade speakers

### 5. Disconnecting

Click "Disconnect" to stop streaming and release the AirPlay device.

## Tips & Best Practices

### Reducing Latency
- Use a wired Ethernet connection for your computer and AirPlay devices when possible
- Close unnecessary applications
- Increase your DAW's buffer size if you experience dropouts

### Multiple Instances
You can load multiple instances of the plugin to stream different tracks to different speakers.

### Troubleshooting

**No devices found:**
- Check network connectivity
- Ensure AirPlay devices are on the same network
- Restart the plugin or DAW
- Check firewall settings

**Audio dropouts:**
- Increase buffer size in the plugin (if available)
- Increase DAW buffer size
- Check network bandwidth
- Move closer to Wi-Fi router

**Connection fails:**
- Some devices may require authentication
- Try restarting the AirPlay device
- Check device compatibility

**Latency is too high:**
- This is normal for network streaming
- Not suitable for real-time monitoring while recording
- Best used for checking mixes, not tracking

## Platform-Specific Notes

### macOS
- Uses native AirPlay APIs for best quality
- Supports AirPlay 2 devices
- May integrate with system AirPlay settings

### Windows
- Uses RAOP protocol
- May require Bonjour service (included with iTunes or available separately)
- Some AirPlay 2 features may not be available

### Linux
- Uses RAOP protocol with Avahi
- Ensure Avahi daemon is running
- Some AirPlay 2 features may not be available

## Technical Specifications

- Audio Format: PCM 16-bit, Stereo
- Sample Rates: 44.1 kHz, 48 kHz
- Latency: ~200-500ms (network dependent)
- Buffer Size: Adaptive (8192 samples default)

## Known Limitations

- Stereo only (no surround sound)
- Network latency makes it unsuitable for real-time tracking
- Some AirPlay 2 devices may have limited compatibility
- Authentication-required devices may not work

## Support

For issues, questions, or feature requests, please refer to the project documentation.
