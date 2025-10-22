# AirPlay Authentication - Quick Start Guide

## 🚀 Quick Start

### Basic Usage (Authentication Enabled by Default)

```cpp
// Authentication is automatically enabled for all connections
AirPlayDevice device("My Speaker", "192.168.1.100", 7000);
RaopClient client;

if (client.connect(device))
{
    // Connected successfully with full authentication
    // RSA key exchange and challenge-response completed
}
```

### Password-Protected Devices

```cpp
AirPlayDevice device("Protected Speaker", "192.168.1.100", 7000);
device.setPassword("speaker123");  // Set the device password

RaopClient client;
if (client.connect(device))
{
    // Connected with password authentication
}
```

### Legacy Devices (No Authentication)

```cpp
// Some older devices don't support authentication
RaopClient client;
client.setUseAuthentication(false);  // Disable authentication

if (client.connect(device))
{
    // Connected without authentication (legacy mode)
}
```

## 🔐 What Happens During Authentication?

1. **OPTIONS** - Client sends challenge, server responds with signature
2. **ANNOUNCE** - Client sends RSA public key and AES settings
3. **SETUP** - RTP transport configuration
4. **RECORD** - Start streaming (with optional encryption)

## 🛠️ Building with Authentication

### Dependencies Required

```bash
# Linux (Ubuntu/Debian)
sudo apt-get install libssl-dev

# macOS (Homebrew)
brew install openssl

# Windows
# Download from: https://slproweb.com/products/Win32OpenSSL.html
```

### CMake Configuration

The project automatically finds and links OpenSSL:

```bash
cd /workspace
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

## 📝 API Reference

### AirPlayDevice Class

```cpp
class AirPlayDevice
{
    void setPassword(const juce::String& password);
    juce::String getPassword() const;
    bool requiresPassword() const;
    void setRequiresPassword(bool requires);
};
```

### RaopClient Class

```cpp
class RaopClient
{
    // Enable/disable authentication
    void setUseAuthentication(bool enable);
    bool requiresAuthentication() const;
    
    // Set password for current device
    void setPassword(const juce::String& password);
};
```

### AirPlayAuth Class (Advanced)

```cpp
class AirPlayAuth
{
    bool initialize();
    juce::String generateChallenge();
    bool verifyResponse(const juce::String& response, ...);
    juce::String getPublicKeyBase64() const;
    bool setupEncryption(const juce::String& aesKey, const juce::String& aesIV);
    int encryptAudioData(const void* data, size_t size, juce::MemoryBlock& encrypted);
    void setPassword(const juce::String& password);
};
```

## 🧪 Testing

### Test with shairport-sync

1. **Install shairport-sync:**
   ```bash
   brew install shairport-sync  # macOS
   # or
   sudo apt-get install shairport-sync  # Linux
   ```

2. **Create password-protected receiver:**
   
   Create `/etc/shairport-sync.conf`:
   ```conf
   general = {
       name = "Test Speaker";
       password = "test123";
   }
   ```

3. **Run shairport-sync:**
   ```bash
   shairport-sync -v
   ```

4. **Connect from FreeCaster:**
   ```cpp
   AirPlayDevice device("Test Speaker", "127.0.0.1", 5000);
   device.setPassword("test123");
   client.connect(device);
   ```

### Test Without Password

```bash
# Simple receiver without password
shairport-sync -a "Simple Speaker"
```

```cpp
// Connect without password
AirPlayDevice device("Simple Speaker", "127.0.0.1", 5000);
client.connect(device);  // No password needed
```

## ⚠️ Troubleshooting

### Build Errors

**Error**: `Could NOT find OpenSSL`
```bash
# Install OpenSSL development headers
sudo apt-get install libssl-dev  # Linux
brew install openssl              # macOS
```

**Error**: `cannot find -lavahi-client`
```bash
# Install Avahi (Linux only)
sudo apt-get install libavahi-client-dev libavahi-common-dev
```

### Runtime Errors

**Error**: `Authentication not initialized`
- **Cause**: OpenSSL initialization failed
- **Fix**: Verify OpenSSL is properly installed

**Error**: `Failed to connect`
- **Cause**: Wrong password or device doesn't support auth
- **Fix**: 
  ```cpp
  // Try without authentication
  client.setUseAuthentication(false);
  ```

**Error**: `Failed to generate RSA key pair`
- **Cause**: OpenSSL error or insufficient entropy
- **Fix**: Check system entropy: `cat /proc/sys/kernel/random/entropy_avail`

## 🎯 Common Scenarios

### Scenario 1: Connect to HomePod
```cpp
AirPlayDevice homepod("Living Room", "192.168.1.50", 7000);
homepod.setPassword("home123");  // If password-protected

RaopClient client;
if (client.connect(homepod))
{
    // Stream audio
}
```

### Scenario 2: Connect to Airport Express
```cpp
// Airport Express usually doesn't need password
AirPlayDevice airport("Kitchen", "192.168.1.60", 5000);

RaopClient client;
if (client.connect(airport))
{
    // Stream audio
}
```

### Scenario 3: Connect to Multiple Devices
```cpp
std::vector<RaopClient> clients(3);
std::vector<AirPlayDevice> devices = {
    {"Speaker 1", "192.168.1.100", 7000},
    {"Speaker 2", "192.168.1.101", 7000},
    {"Speaker 3", "192.168.1.102", 7000}
};

for (size_t i = 0; i < devices.size(); ++i)
{
    if (clients[i].connect(devices[i]))
    {
        DBG("Connected to " + devices[i].getDeviceName());
    }
}
```

## 📊 Performance Tips

### Connection Time
- **With Auth**: ~100-150ms (includes RSA key generation)
- **Without Auth**: ~50ms
- **Tip**: Cache connections instead of reconnecting frequently

### Encryption Overhead
- **AES-128-CBC**: <1% CPU overhead
- **Tip**: Use hardware AES acceleration when available

### Memory Usage
- **Per Connection**: ~1-2KB for auth data
- **Tip**: Minimal impact, no special handling needed

## 🔒 Security Best Practices

1. **Don't Store Passwords in Code**
   ```cpp
   // ❌ Bad
   device.setPassword("hardcoded123");
   
   // ✅ Good
   juce::String password = getUserPasswordFromKeychain();
   device.setPassword(password);
   ```

2. **Clear Sensitive Data**
   ```cpp
   // When done
   device.setPassword("");  // Clear password
   ```

3. **Validate Certificates** (Future Enhancement)
   ```cpp
   // TODO: Implement certificate pinning
   // client.enableCertificateValidation(true);
   ```

## 📚 Additional Resources

- **Full Documentation**: See `AIRPLAY_AUTHENTICATION.md`
- **RTSP Protocol**: See authentication flow in main documentation
- **OpenSSL API**: https://www.openssl.org/docs/
- **AirPlay Protocol**: Reverse-engineered from shairport-sync

## ✅ Checklist

Before deploying authentication:

- [ ] OpenSSL installed and linked
- [ ] Build succeeds without errors
- [ ] Tested with password-protected device
- [ ] Tested with non-password device
- [ ] Tested with authentication disabled
- [ ] Error handling implemented
- [ ] Passwords not hardcoded
- [ ] Performance acceptable (<150ms connection time)

---

**Need Help?** Check `AIRPLAY_AUTHENTICATION.md` for detailed information.
