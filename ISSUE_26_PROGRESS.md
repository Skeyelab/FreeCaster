# Issue #26 Progress Report

## Summary
Fixed RTSP response reading (original issue), but discovered deeper protocol issues during testing.

## Original Issue Status: ✅ FIXED
**Issue**: AirPlay devices not responding to RTSP requests  
**Root Cause**: Non-blocking socket read returning before device responded  
**Solution**: Added `waitUntilReady()` call before reading responses  
**Commit**: `f49b453`

## Current Status

### ✅ Working
- TCP connection establishment
- RTSP OPTIONS request/response handling
- Reading and parsing RTSP responses
- Apple-Response detection and conditional auth

### ❌ Failing
- ANNOUNCE request: 403 Forbidden on native Sonos devices
- SETUP request: 500 Internal Error on airsonos bridge devices
- OPTIONS fails on some devices (LIB-2833): 403 Forbidden

## Testing Results

### Device: Native Sonos Move (row 1)
```
OPTIONS: ✅ 200 OK (no Apple-Response)
ANNOUNCE: ❌ 403 Forbidden
SETUP: Never reached
```

### Device: Airsonos Bridge (row 3)
```
OPTIONS: ✅ 200 OK (with Apple-Response)
ANNOUNCE: ✅ 200 OK
SETUP: ❌ 500 Internal Error
```

### Device: LIB-2833 (row 0)
```
OPTIONS: ❌ 403 Forbidden
```

## Attempted Fixes

1. ✅ Wait for socket readiness before reading
2. ✅ Conditional auth fields based on Apple-Response
3. ✅ Always send auth fields
4. ✅ Changed SDP connection address from device IP to 127.0.0.1
5. ✅ Removed `interleaved` parameter from SETUP Transport header
6. ✅ Added detailed logging for debugging

## Possible Root Causes

### For 403 on ANNOUNCE:
- Incorrect SDP format
- Missing required SDP fields
- Wrong RSA key encoding
- Apple-Response verification issue

### For 500 on SETUP:
- Incorrect Transport header format
- Missing required headers
- Device-specific bug (airsonos bridge)
- Port configuration issue

## Recommendations

1. **Reference Implementation**: Compare with shairport-sync SDP generation
2. **Packet Capture**: Analyze actual AirPlay connection to Sonos
3. **Test Different Devices**: Try other AirPlay targets (Apple TV, HomePod)
4. **Protocol Documentation**: Find official/unofficial RAOP protocol spec

## Next Steps

1. Create new issue for ANNOUNCE/SETUP failures
2. Research shairport-sync implementation
3. Test with different AirPlay devices
4. Consider temporary workaround (document limitation)

## Commits Made

- `f49b453` - Fix RTSP response reading - wait for socket readiness
- `3cc75f5` - Add logging for Apple-Response handling
- `b91b46e` - Only include auth fields if device sent Apple-Response
- `6c2d3a6` - Remove interleaved parameter from SETUP
- `8cf4606` - Always send auth fields and fix SDP connection address
- `2c88f63` - Improve Apple-Response handling logic

## Conclusion

Issue #26 (RTSP reading) is **technically fixed** - we now successfully read responses from devices. However, we've uncovered deeper protocol compatibility issues that prevent full connections. These appear to be related to SDP formatting and authentication handling rather than the original socket reading problem.

