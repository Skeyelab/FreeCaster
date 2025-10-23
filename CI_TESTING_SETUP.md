# CI Testing Setup for FreeCaster

## Overview

Continuous Integration (CI) has been configured to automatically build and run the FreeCaster unit test suite on every push and pull request to the `main` branch.

## What Was Added

### GitHub Actions Workflow Updates

**File**: `.github/workflows/build.yml`

The existing Linux build workflow was enhanced with:

1. **Build Unit Tests** step - Compiles the `FreeCasterTests` executable
2. **Run Unit Tests** step - Executes all 4,402 test assertions
3. **Added `libfontconfig1-dev`** to dependencies (required for JUCE graphics)
4. **FontConfig verification** in dependency check

### CI Workflow Steps

```yaml
- Build Linux Plugin         # Existing: Build FreeCaster VST/Standalone
- Build Unit Tests           # NEW: Compile test executable
- Run Unit Tests             # NEW: Execute tests and validate results
- Create clean distribution  # Existing: Package artifacts
- Upload build artifacts     # Existing: Archive builds
```

## Test Execution in CI

When the workflow runs, you'll see output like:

```
Building FreeCaster unit tests...
cd build
make FreeCasterTests -j$(nproc)

Running FreeCaster unit tests...
cd build
./FreeCasterTests

=== Test Summary ===
Total tests: 4402
Passed: 4402
Failed: 0
===================

✅ All tests passed!
```

### Failure Handling

If any test fails:
- The CI job will **fail** and stop the workflow
- The exit code will be `1`
- GitHub will mark the check as ❌ failed
- You'll see detailed output showing which tests failed

## Benefits

✅ **Automated Validation** - Every commit is tested automatically  
✅ **Early Bug Detection** - Catch regressions before merge  
✅ **No Manual Testing** - Tests run without human intervention  
✅ **Fast Feedback** - Results in < 2 minutes  
✅ **Pull Request Checks** - Tests run on all PRs before merge  
✅ **Build Confidence** - Ensure code quality before release  

## Platforms

Currently running on:
- ✅ **Linux** (Ubuntu 24.04) - Full test suite
- ⏳ **macOS** - Not yet integrated (can be added)
- ⏳ **Windows** - Not yet integrated (can be added)

## Test Coverage in CI

All test suites run automatically:

1. **RaopClientTests** - RTSP/RTP protocol validation
2. **StreamBufferTests** - Thread-safe buffer operations
3. **AudioEncoderTests** - Audio format encoding
4. **AirPlayDeviceTests** - Device model validation

Total: **4,402 test assertions**

## Dependencies Installed

The CI environment installs all required packages:

```bash
# Core build tools
build-essential, cmake, git, pkg-config

# Audio libraries
libasound2-dev, libjack-jackd2-dev

# Graphics & UI
libfreetype6-dev, libfontconfig1-dev, libgl1-mesa-dev
libx11-dev, libxinerama-dev, libxext-dev, libxrandr-dev
libxcursor-dev, libxcomposite-dev, libgtk-3-dev

# Networking
libcurl4-openssl-dev, libavahi-client-dev, libavahi-common-dev

# Security
libssl-dev
```

## Viewing Test Results

### On GitHub

1. Go to your repository
2. Click **Actions** tab
3. Select a workflow run
4. Expand **Run Unit Tests** step
5. View detailed test output

### Locally

To run the same tests locally:

```bash
cd build
make FreeCasterTests -j$(nproc)
./FreeCasterTests
```

## Adding Tests to macOS CI

To enable tests on macOS, add similar steps to the `build-macos` job:

```yaml
- name: Build Unit Tests
  run: |
    cd build
    make FreeCasterTests -j$(sysctl -n hw.ncpu)

- name: Run Unit Tests
  run: |
    cd build
    ./FreeCasterTests
```

## Adding Tests to Windows CI

To enable tests on Windows (when available), add to `build-windows` job:

```yaml
- name: Build Unit Tests
  shell: cmd
  run: |
    cd build
    cmake --build . --target FreeCasterTests --config Release

- name: Run Unit Tests
  shell: cmd
  run: |
    cd build
    Release\FreeCasterTests.exe
```

## Troubleshooting CI Failures

### Test Failures

If tests fail in CI but pass locally:
- Check for environment differences
- Look for timing-dependent tests (threading)
- Verify random seed (shown in test output)

### Build Failures

If tests fail to build:
- Ensure all dependencies are installed
- Check CMake configuration
- Verify JUCE modules are available

### Missing Dependencies

If `libfontconfig1-dev` or other packages are missing:
- Update `.github/workflows/build.yml`
- Add to the `Install Linux dependencies` step
- Re-run the workflow

## Test Performance

Typical CI execution times:
- Build FreeCasterTests: **~30 seconds**
- Run FreeCasterTests: **< 1 second**
- Total overhead: **~30 seconds**

This is a small addition to the overall build time (~2-3 minutes total).

## Future Enhancements

Potential improvements:
- [ ] Add test result caching
- [ ] Generate test coverage reports
- [ ] Add macOS test execution
- [ ] Add Windows test execution
- [ ] Upload test results as artifacts
- [ ] Add test result badge to README
- [ ] Run tests on multiple Ubuntu versions
- [ ] Add performance benchmarking tests

## Status Badge

Add this to your README to show test status:

```markdown
![Tests](https://github.com/YOUR_USERNAME/FreeCaster/workflows/Build%20FreeCaster/badge.svg)
```

## Summary

The FreeCaster CI pipeline now includes comprehensive automated testing:
- ✅ 4,402 test assertions run on every commit
- ✅ Catches bugs before they reach production
- ✅ Validates protocol, threading, encoding, and models
- ✅ Zero additional manual work required
- ✅ Fast feedback loop for developers

All tests must pass for the build to succeed! 🎉
