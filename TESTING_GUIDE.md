# RTP Audio Streaming - Testing Guide

## Quick Start Testing

This guide helps you test the RTP audio streaming implementation without needing to build the full plugin.

---

## Prerequisites

### Required Software
- Wireshark (for packet capture)
- An AirPlay device (HomePod, Apple TV, etc.)
- Network access to the device

### Optional Tools
- `tcpdump` for command-line packet capture
- `nmap` to discover AirPlay ports
- Network analyzer (Little Snitch, etc.)

---

## Test 1: Verify RTSP Connection

### Goal
Confirm that FreeCaster can establish RTSP connection and negotiate ports.

### Steps

1. **Find your AirPlay device IP**
   ```bash
   # On macOS:
   dns-sd -B _raop._tcp .
   
   # Or use avahi on Linux:
   avahi-browse -r _raop._tcp
   ```

2. **Expected output**
   ```
   freecaster._raop._tcp    default
       hostname = [airplay-device.local]
       address = [192.168.1.100]
       port = [7000]
   ```

3. **Test manual RTSP connection**
   ```bash
   telnet 192.168.1.100 7000
   
   # Type:
   OPTIONS * RTSP/1.0
   CSeq: 1
   
   # Press Enter twice
   ```

4. **Expected response**
   ```
   RTSP/1.0 200 OK
   CSeq: 1
   Public: ANNOUNCE, SETUP, RECORD, PAUSE, FLUSH, TEARDOWN, OPTIONS, GET_PARAMETER, SET_PARAMETER
   ```

### Pass Criteria
✅ RTSP server responds with 200 OK  
✅ `Public` header lists SETUP, RECORD, TEARDOWN

---

## Test 2: Wireshark RTP Packet Capture

### Goal
Verify that RTP packets are being sent with correct format.

### Steps

1. **Start Wireshark**
   ```bash
   sudo wireshark
   ```

2. **Select your network interface** (WiFi or Ethernet)

3. **Set filter**
   ```
   ip.dst == 192.168.1.100 && udp
   ```
   *(Replace with your AirPlay device IP)*

4. **Start capture**

5. **Run FreeCaster and play audio**

6. **Look for RTP packets** in the capture

### What to Look For

**Packet Structure:**
```
Frame: 1420 bytes on wire
Ethernet II
Internet Protocol Version 4
    Src: 192.168.1.50
    Dst: 192.168.1.100
User Datagram Protocol
    Src Port: 54320 (dynamic)
    Dst Port: 6000 (server audio port)
Real-Time Transport Protocol
    Version: 2
    Padding: False
    Extension: False
    Marker: False
    Payload type: 96 (dynamic)
    Sequence number: 1234
    Timestamp: 704
    SSRC: 0x12345678
    Payload: 1408 bytes
```

### Validation Checklist

- [ ] RTP version = 2
- [ ] Payload type = 96
- [ ] Sequence numbers increment by 1
- [ ] No gaps in sequence (except on wraparound)
- [ ] Timestamps increase monotonically
- [ ] SSRC stays constant for session
- [ ] Packet sizes ≤ 1420 bytes
- [ ] No IP fragmentation

### Pass Criteria
✅ All packets have correct RTP structure  
✅ Sequence numbers increment properly  
✅ No fragmented packets

---

## Test 3: Audio Playback Verification

### Goal
Verify that audio actually plays on the AirPlay device.

### Setup

1. **Connect speaker/headphones** to AirPlay device
2. **Load FreeCaster** in your DAW
3. **Select test audio**:
   - Simple sine wave (440Hz)
   - Pink noise
   - Music track

### Test Cases

#### Test 3.1: Sine Wave Test

**Expected**: Clear 440Hz tone without distortion

```
DAW Settings:
- Sample Rate: 44.1kHz
- Bit Depth: 16-bit
- Channels: Stereo
- Buffer: 512 samples
```

**Pass Criteria**:
- [ ] Tone plays continuously
- [ ] No clicks/pops
- [ ] No dropouts
- [ ] Latency < 500ms

#### Test 3.2: Pink Noise Test

**Expected**: Consistent noise across frequency spectrum

**Pass Criteria**:
- [ ] Even distribution of frequencies
- [ ] No audible gaps
- [ ] No clipping

#### Test 3.3: Music Playback

**Expected**: High-quality audio reproduction

**Pass Criteria**:
- [ ] Music plays clearly
- [ ] No artifacts or distortion
- [ ] Stereo imaging preserved
- [ ] Dynamics intact

### Failure Modes

| Symptom | Possible Cause | Fix |
|---------|----------------|-----|
| No audio | Wrong server port | Check SETUP response |
| Choppy audio | Packet loss | Check network quality |
| Distorted audio | Wrong sample rate | Verify SR matches |
| Clicks/pops | Buffer underrun | Increase buffer size |
| Delayed audio | Network latency | Expected (200-500ms) |

---

## Test 4: Stress Testing

### Goal
Verify stability under various conditions.

### Test 4.1: Long Duration

**Duration**: 30 minutes continuous playback

**Monitor**:
- [ ] No memory leaks
- [ ] CPU usage stable
- [ ] No dropouts
- [ ] No crashes

### Test 4.2: Network Stress

**Scenarios**:
1. Start download while streaming
2. Multiple devices on network
3. Move device between rooms
4. Weak WiFi signal

**Pass Criteria**:
- [ ] Graceful degradation
- [ ] Recovers from interruptions
- [ ] No crashes

### Test 4.3: Reconnection

**Steps**:
1. Start playback
2. Disconnect device (power off)
3. Reconnect device
4. Resume playback

**Pass Criteria**:
- [ ] Detects disconnection
- [ ] Allows manual reconnection
- [ ] Resumes without crash

---

## Test 5: Multi-Device Testing

### Goal
Verify compatibility across different AirPlay devices.

### Devices to Test

| Device | Model | Expected Result |
|--------|-------|-----------------|
| HomePod | Any | ✅ Should work |
| HomePod mini | Any | ✅ Should work |
| Apple TV | 4K, HD | ✅ Should work |
| AirPort Express | 2nd gen | ✅ Should work |
| Third-party | Various | ⚠️ May need encryption |

### For Each Device

- [ ] Discovery works
- [ ] Connection succeeds
- [ ] Audio plays
- [ ] No error messages
- [ ] Latency acceptable

### Known Issues

**Encrypted Devices**:
- Some third-party speakers require AES encryption
- Will fail with "Connection refused" or "Auth required"
- **Fix**: Implement encryption (future work)

**Password-Protected**:
- Devices with PINs won't work yet
- **Fix**: Implement RSA auth (future work)

---

## Test 6: Sample Rate Testing

### Goal
Verify different sample rates work correctly.

### Test Matrix

| Sample Rate | Bit Depth | Channels | Expected |
|-------------|-----------|----------|----------|
| 44.1 kHz | 16-bit | Stereo | ✅ Primary |
| 48 kHz | 16-bit | Stereo | ✅ Should work |
| 44.1 kHz | 16-bit | Mono | ✅ Should work |
| 48 kHz | 24-bit | Stereo | ⚠️ Downsampled to 16-bit |
| 96 kHz | 24-bit | Stereo | ⚠️ Requires conversion |

### Steps

1. Set DAW to test sample rate
2. Load FreeCaster
3. Play test audio
4. Verify playback quality

### Pass Criteria

- [ ] 44.1kHz works perfectly
- [ ] 48kHz works (may need conversion)
- [ ] Mono works
- [ ] No crashes on unsupported rates

---

## Test 7: Code Coverage (Automated)

### Unit Tests Needed

```cpp
// Test RTP header size
TEST(RtpHeader, CorrectSize) {
    EXPECT_EQ(sizeof(RtpHeader), 12);
}

// Test sequence number wrapping
TEST(RtpClient, SequenceNumberWraps) {
    RaopClient client;
    client.sequenceNumber = 65535;
    client.sendAudio(testData, 44100, 2);
    EXPECT_EQ(client.sequenceNumber, 0);
}

// Test timestamp increment
TEST(RtpClient, TimestampIncrement) {
    RaopClient client;
    client.samplesTransmitted = 0;
    
    // Send 1408 bytes (704 stereo samples)
    client.sendAudio(testData, 44100, 2);
    
    EXPECT_EQ(client.samplesTransmitted, 704);
}

// Test network byte order
TEST(RtpHeader, BigEndian) {
    RtpHeader header;
    header.sequenceNumber = 0x1234;
    
    uint8_t* bytes = (uint8_t*)&header;
    EXPECT_EQ(bytes[2], 0x12);  // High byte first
    EXPECT_EQ(bytes[3], 0x34);  // Low byte second
}
```

---

## Debugging Tips

### Enable Debug Logging

Add to `RaopClient.cpp`:

```cpp
#define DEBUG_RTP 1

#if DEBUG_RTP
    #define RTP_DBG(x) DBG("[RTP] " << x)
#else
    #define RTP_DBG(x)
#endif

// In constructor:
RTP_DBG("SSRC: 0x" << String::toHexString(ssrc));

// In sendAudio:
RTP_DBG("Seq: " << sequenceNumber << 
        " TS: " << samplesTransmitted << 
        " Size: " << payloadSize);

// In sendSetup:
RTP_DBG("Server ports: " << serverAudioPort << "/" << 
        serverControlPort << "/" << serverTimingPort);
```

### Wireshark Display Filters

**Show only RTP packets**:
```
rtp.version == 2
```

**Show packet loss** (gaps in sequence):
```
rtp.seq < rtp.prev_seq
```

**Show by payload type**:
```
rtp.p_type == 96
```

**Follow RTP stream**:
```
rtp.ssrc == 0x12345678
```

### tcpdump Commands

**Capture RTP packets**:
```bash
sudo tcpdump -i en0 -w rtp_capture.pcap \
    'dst host 192.168.1.100 and udp'
```

**Read capture**:
```bash
tcpdump -r rtp_capture.pcap -v
```

**Count packets**:
```bash
tcpdump -r rtp_capture.pcap | wc -l
```

---

## Performance Benchmarks

### Expected Metrics

**CPU Usage**:
- Idle: < 1%
- Streaming 44.1kHz: < 5%
- Streaming 48kHz: < 7%

**Memory Usage**:
- Base: ~10 MB
- Streaming: ~12 MB
- Per-second increase: < 100 KB (should be stable)

**Network Bandwidth**:
- 44.1kHz stereo: ~1.45 Mbps
- 48kHz stereo: ~1.58 Mbps

**Latency**:
- LAN: 200-300ms
- WiFi: 300-500ms
- Over Internet: Not recommended

### How to Measure

**CPU/Memory** (macOS):
```bash
top -pid $(pgrep -f FreeCaster)
```

**Network** (macOS):
```bash
nettop -p FreeCaster
```

**Latency**:
1. Play click track in DAW
2. Record output from AirPlay device
3. Measure time difference
4. Subtract audio processing latency

---

## Automated Testing Script

```bash
#!/bin/bash
# test_rtp_streaming.sh

DEVICE_IP="192.168.1.100"
DEVICE_PORT="7000"

echo "Testing RTP Streaming Implementation"
echo "====================================="

# Test 1: RTSP Connection
echo -n "Test 1: RTSP Connection... "
nc -z $DEVICE_IP $DEVICE_PORT
if [ $? -eq 0 ]; then
    echo "✅ PASS"
else
    echo "❌ FAIL"
    exit 1
fi

# Test 2: RTSP OPTIONS
echo -n "Test 2: RTSP OPTIONS... "
response=$(echo -e "OPTIONS * RTSP/1.0\r\nCSeq: 1\r\n\r\n" | nc $DEVICE_IP $DEVICE_PORT)
if echo "$response" | grep -q "200 OK"; then
    echo "✅ PASS"
else
    echo "❌ FAIL"
    exit 1
fi

# Test 3: Supported methods
echo -n "Test 3: SETUP/RECORD supported... "
if echo "$response" | grep -q "SETUP"; then
    echo "✅ PASS"
else
    echo "❌ FAIL"
    exit 1
fi

echo ""
echo "All basic tests passed! ✅"
echo "Next: Test with actual FreeCaster plugin"
```

**Usage**:
```bash
chmod +x test_rtp_streaming.sh
./test_rtp_streaming.sh
```

---

## Regression Testing

### Before Each Release

- [ ] All unit tests pass
- [ ] Integration tests pass
- [ ] Manual testing on 3+ devices
- [ ] 30-minute stress test
- [ ] Network interruption test
- [ ] Sample rate matrix
- [ ] Memory leak check (Valgrind/Instruments)

### Test Results Template

```markdown
## Test Report - RTP Streaming

**Date**: YYYY-MM-DD
**Version**: X.Y.Z
**Tester**: Name

### Environment
- OS: macOS 14.0
- DAW: Logic Pro 10.8
- Network: WiFi 5GHz
- Device: HomePod mini

### Results

| Test | Status | Notes |
|------|--------|-------|
| RTSP Connection | ✅ | Connected in 250ms |
| RTP Packets | ✅ | All packets well-formed |
| Audio Playback | ✅ | Clear, no dropouts |
| Long Duration | ✅ | 60min continuous |
| Reconnection | ✅ | Recovered after disconnect |

### Issues Found
None

### Performance
- CPU: 3.2% average
- Memory: 11.5 MB stable
- Latency: 280ms
- Packet loss: 0.01%

### Conclusion
✅ PASS - Ready for release
```

---

## Getting Help

### Common Questions

**Q: No audio plays, but Wireshark shows packets**  
A: Check if device requires encryption (not yet implemented)

**Q: Connection fails immediately**  
A: Verify device IP/port, check firewall

**Q: Choppy audio**  
A: Check WiFi signal, reduce network load

**Q: High CPU usage**  
A: Unexpected - file bug report

### Filing Bug Reports

Include:
1. Test case that fails
2. Wireshark capture
3. Debug logs
4. Device model/firmware
5. Network topology

---

## Success Criteria

For this implementation to be considered "working":

✅ RTP packets sent with correct format  
✅ Audio plays on at least 3 different devices  
✅ No crashes during 30-minute test  
✅ Packet loss < 1%  
✅ Latency < 500ms  
✅ CPU usage < 10%  
✅ Memory stable (no leaks)

**Current Status**: Core implementation complete, needs device testing

---

**Next Steps**:
1. Run Test 1-3 with real hardware
2. Fix any issues found
3. Run stress tests (Test 4)
4. Test multiple devices (Test 5)
5. Document results
6. Implement encryption if needed
