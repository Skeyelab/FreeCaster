# FreeCaster Unit Tests

This directory contains comprehensive unit tests for the FreeCaster audio streaming plugin, built using JUCE's `juce::UnitTest` framework.

## Test Coverage

### RaopClientTests.cpp
Tests for RTSP/RTP protocol implementation:
- **RTSP Response Parsing**: Valid responses, error codes, multi-line headers, body content
- **Transport Header Parsing**: Standard format, missing timing ports, various formatting
- **RTP Header Construction**: Version flags, payload types, sequence numbers, timestamps

### StreamBufferTests.cpp
Tests for thread-safe circular buffer:
- **Basic Operations**: Write, read, available space calculations
- **Edge Cases**: Buffer overflow, underflow, wrap-around behavior
- **Thread Safety**: Concurrent read/write operations
- **Clear Operations**: State management during active operations

### AudioEncoderTests.cpp
Tests for audio format conversion:
- **PCM 16-bit Encoding**: Accuracy, sample limits, conversion precision
- **PCM 24-bit Encoding**: Format validation, size calculations
- **Sample Rates**: 44.1kHz, 48kHz support
- **Channel Interleaving**: Stereo channel ordering
- **Float to Int Conversion**: Precision validation across value ranges

### AirPlayDeviceTests.cpp
Tests for device data model:
- **Device Construction**: Parameterized and default construction
- **Validation**: isValid() logic for required fields
- **Property Management**: Getters, setters, state consistency
- **Password Handling**: Password requirement flags
- **Equality**: Comparison and copying behavior

## Building Tests

```bash
# Configure CMake (first time only)
mkdir -p build && cd build
cmake ..

# Build the test executable
make FreeCasterTests

# Or build with multiple cores
make FreeCasterTests -j$(nproc)
```

## Running Tests

```bash
# Run all tests
cd build
./FreeCasterTests

# Tests will output detailed results including:
# - Individual test pass/fail status
# - Summary statistics
# - Random seed for reproducibility
```

## Test Results

A successful test run will show:
```
=== Test Summary ===
Total tests: 4402
Passed: 4402
Failed: 0
===================
```

Exit code:
- `0` = All tests passed
- `1` = One or more tests failed

## Test Structure

Each test suite follows this pattern:

```cpp
class MyComponentTests : public juce::UnitTest
{
public:
    MyComponentTests() : juce::UnitTest("MyComponent") {}
    
    void runTest() override
    {
        beginTest("Test description");
        {
            // Test implementation
            expect(condition, "Failure message");
            expectEquals(actual, expected, "Failure message");
        }
    }
};

static MyComponentTests myComponentTests;
```

## Key Features

1. **No Hardware Required**: All tests run without AirPlay devices or network
2. **Fast Execution**: Complete test suite runs in < 1 second
3. **Cross-Platform**: Tests work on Linux, macOS, and Windows
4. **Thread Safety Validation**: Concurrent operations tested with real threads
5. **Comprehensive Coverage**: 4,400+ individual test assertions

## Adding New Tests

1. Create a new test file in `Tests/` directory
2. Include it in `Tests/TestMain.cpp`
3. Add the source files to `CMakeLists.txt` under `FreeCasterTests` target
4. Follow the JUCE UnitTest pattern shown above

## CI Integration

To integrate with GitHub Actions, add to your workflow:

```yaml
- name: Run Unit Tests
  run: |
    cd build
    ./FreeCasterTests
```

## Dependencies

Tests link against:
- `juce::juce_audio_basics` - Audio buffer support
- `juce::juce_core` - Core JUCE functionality
- `juce::juce_events` - Event system
- `juce::juce_graphics` - Graphics types (Colour)
- `OpenSSL::SSL` - Cryptography for authentication
- `OpenSSL::Crypto` - Crypto primitives

## Test Philosophy

- **Unit Tests**: Each test focuses on a single component in isolation
- **No Mocking**: Direct testing of actual implementations
- **Edge Cases**: Extensive coverage of boundary conditions
- **Thread Safety**: Real concurrent access patterns
- **Documentation**: Tests serve as usage examples

## Troubleshooting

**Build fails with missing headers**: Install JUCE dependencies
```bash
sudo apt-get install libasound2-dev libfreetype6-dev libx11-dev \
  libxinerama-dev libxrandr-dev libxcursor-dev libxcomposite-dev \
  mesa-common-dev freeglut3-dev libcurl4-openssl-dev libfontconfig1-dev
```

**Linker errors**: Ensure all required JUCE modules are linked in CMakeLists.txt

**Tests hang**: Check for deadlocks in concurrent test cases

**Random failures**: Tests use deterministic random seed (shown in output)
