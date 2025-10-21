# Testing FreeCaster with Simulated AirPlay Devices

## Option 1: macOS Built-in AirPlay Receiver ⭐ RECOMMENDED

**Easiest option - no installation needed!**

1. Open **System Settings**
2. Navigate to **General** → **AirDrop & Handoff**
3. Enable **"AirPlay Receiver"**
4. Set **"Allow AirPlay for:"** to "Everyone" or "Current User"
5. Your Mac will appear as an AirPlay device named after your computer

## Option 2: Shairport Sync (Open Source)

Install via Homebrew:

```bash
brew install shairport-sync
```

Run it:

```bash
shairport-sync -a "Test Speaker"
```

This creates a virtual AirPlay device called "Test Speaker" that outputs to your Mac's audio.

Advanced options:
```bash
# Run with custom name
shairport-sync -a "Studio Monitor"

# Run with specific audio backend
shairport-sync -a "Test Device" -o alsa

# Run in foreground with verbose output
shairport-sync -v -a "Debug Speaker"
```

Stop with Ctrl+C when done.

## Option 3: Multiple Devices with Docker

Create multiple virtual AirPlay devices:

```bash
# Install Docker Desktop for Mac first
brew install --cask docker

# Run multiple containers
docker run -d --name airplay1 --net=host kevineye/shairport-sync -a "Speaker 1"
docker run -d --name airplay2 --net=host kevineye/shairport-sync -a "Speaker 2"
docker run -d --name airplay3 --net=host kevineye/shairport-sync -a "Speaker 3"

# Stop when done
docker stop airplay1 airplay2 airplay3
docker rm airplay1 airplay2 airplay3
```

## Option 4: AirServer (Commercial, Has Trial)

- Download from: https://www.airserver.com/
- 7-day free trial
- Supports AirPlay 2
- More features but costs $19.99

## Testing FreeCaster

### 1. Start Standalone App

```bash
cd ~/FreeCaster/build
./FreeCaster_artefacts/Release/Standalone/FreeCaster.app/Contents/MacOS/FreeCaster
```

### 2. Or Load in DAW

1. Open your DAW
2. Load FreeCaster on a track
3. Play some audio

### 3. What to Look For

✅ **Device List**: Should show your AirPlay receiver(s)
✅ **Connection**: Should connect without errors
✅ **Audio Streaming**: Should hear audio on the AirPlay device
⚠️ **Latency**: Expect 200-500ms delay (normal for network audio)

### Troubleshooting

**No devices found?**
- Check both FreeCaster and receiver are on same network
- Restart FreeCaster
- Check firewall settings
- Run `dns-sd -B _raop._tcp` to verify AirPlay devices are discoverable

**Can't connect?**
- Some devices require authentication (not yet implemented)
- Check Console.app for error messages
- Try the macOS built-in receiver first (most compatible)

**No audio?**
- Check the receiver's volume
- Verify audio is playing in DAW/standalone
- Check macOS Sound settings

### Quick Test Script

```bash
#!/bin/bash
# Test FreeCaster discovery

echo "Scanning for AirPlay devices..."
dns-sd -B _raop._tcp local. &
DNS_PID=$!

sleep 5
kill $DNS_PID

echo ""
echo "If you see devices above, FreeCaster should find them too!"
```

## Recommended Testing Order

1. ✅ Enable macOS AirPlay Receiver (simplest)
2. ✅ Test FreeCaster Standalone → Connect → Play audio
3. ✅ Install shairport-sync for multiple devices
4. ✅ Test in your DAW
5. ✅ Test with real AirPlay speakers if available

## Performance Testing

```bash
# Monitor network traffic
nettop -m tcp

# Check CPU usage
top -o cpu | grep FreeCaster

# Monitor audio glitches
# Look in DAW's performance meter
```

---

**Pro Tip**: Use shairport-sync with verbose mode to see exactly what's happening:
```bash
shairport-sync -v -a "Debug Speaker" 2>&1 | grep -i "audio\|connect\|stream"
```
