# CI Testing Quick Reference

## What Happens on Every Commit

When you push code or create a PR, GitHub Actions will:

```
1. ✅ Checkout code
2. ✅ Install dependencies (including libfontconfig1-dev)
3. ✅ Build FreeCaster plugin
4. ✅ Build FreeCasterTests executable    ← NEW
5. ✅ Run 4,402 test assertions           ← NEW
6. ✅ Create distribution packages
7. ✅ Upload artifacts
```

## CI Test Output

You'll see this in the GitHub Actions log:

```bash
Building FreeCaster unit tests...
cd build
make FreeCasterTests -j$(nproc)
[100%] Built target FreeCasterTests

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

## How to View Results

### On GitHub
1. Go to your repo
2. Click **Actions** tab  
3. Select the workflow run
4. Expand **"Run Unit Tests"** step

### PR Checks
Every PR will show:
- ✅ **Build FreeCaster** - All checks passed
- ❌ **Build FreeCaster** - Tests failed

## What Triggers CI

- Push to `main` branch
- Pull request to `main` branch
- Manual workflow dispatch (if enabled)

## Test Categories Validated

| Test Suite | Tests | What It Validates |
|------------|-------|-------------------|
| RaopClientTests | 15 | RTSP/RTP protocol parsing |
| StreamBufferTests | 7 | Thread-safe buffer operations |
| AudioEncoderTests | 7 | PCM encoding accuracy |
| AirPlayDeviceTests | 7 | Device model validation |
| **Total** | **36 suites** | **4,402 assertions** |

## If Tests Fail

The workflow will:
1. Stop immediately after test failure
2. Show which test failed
3. Mark the check as ❌ failed
4. Prevent merge (if branch protection enabled)

Example failure output:
```
!!! Test 5 failed: Buffer should be empty
Expected value: 0, Actual value: 512

FAILED!! 1 test failed, out of 5

❌ Tests failed!
Error: Process completed with exit code 1.
```

## Local vs CI Testing

### Run Locally
```bash
cd build
make FreeCasterTests -j$(nproc)
./FreeCasterTests
```

### Same as CI
The local tests are **identical** to CI tests. If they pass locally, they'll pass in CI (assuming same environment).

## Debugging CI Failures

### 1. Reproduce Locally
```bash
# Use the same Ubuntu version as CI
docker run -it ubuntu:24.04
# Install dependencies (from workflow)
# Run tests
```

### 2. Check Logs
Look for:
- Failed test names
- Expected vs actual values
- Stack traces (if any)

### 3. Common Issues
- **Timing**: Thread-safety tests may be timing-sensitive
- **Environment**: Missing dependencies
- **Random seed**: Check the seed in test output

## Performance

| Step | Time |
|------|------|
| Build tests | ~30s |
| Run tests | <1s |
| **Total overhead** | **~30s** |

## Dependencies Added

New dependency for test builds:
```yaml
libfontconfig1-dev  # Font configuration (JUCE graphics)
```

All other dependencies were already present.

## Future Improvements

Potential enhancements:
- [ ] Test coverage reporting
- [ ] Performance benchmarks
- [ ] macOS test execution
- [ ] Windows test execution
- [ ] Test result artifacts
- [ ] Slack/Discord notifications

## Status

✅ **Active** - Tests run on every commit  
✅ **Linux** - Fully integrated  
⏳ **macOS** - Can be added  
⏳ **Windows** - Can be added  

## Quick Commands

```bash
# View workflow file
cat .github/workflows/build.yml

# Validate YAML
python3 -c "import yaml; yaml.safe_load(open('.github/workflows/build.yml'))"

# Run tests locally
cd build && ./FreeCasterTests

# Build from scratch
rm -rf build && mkdir build && cd build && cmake .. && make FreeCasterTests
```

## Documentation

- **Tests/README.md** - Test suite documentation
- **TESTING_FRAMEWORK_SUMMARY.md** - Implementation details
- **CI_TESTING_SETUP.md** - Complete CI setup guide
- **This file** - Quick reference

---

**Questions?** Check the full documentation in `CI_TESTING_SETUP.md`
