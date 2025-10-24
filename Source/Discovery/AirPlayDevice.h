#pragma once
#include <JuceHeader.h>

class AirPlayDevice
{
public:
    AirPlayDevice() = default;
    AirPlayDevice(const juce::String& name, const juce::String& host, int port)
        : deviceName(name), hostAddress(host), port(port) {}

    juce::String getDeviceName() const { return deviceName; }
    juce::String getHostAddress() const { return hostAddress; }
    int getPort() const { return port; }
    juce::String getDeviceId() const { return deviceId; }
    juce::String getPassword() const { return password; }
    bool requiresPassword() const { return needsPassword; }
    juce::String getServerPublicKey() const { return serverPublicKey; }

    void setDeviceName(const juce::String& name) { deviceName = name; }
    void setHostAddress(const juce::String& host) { hostAddress = host; }
    void setPort(int p) { port = p; }
    void setDeviceId(const juce::String& id) { deviceId = id; }
    void setPassword(const juce::String& pwd) { password = pwd; needsPassword = pwd.isNotEmpty(); }
    void setRequiresPassword(bool requires) { needsPassword = requires; }
    void setServerPublicKey(const juce::String& pk) { serverPublicKey = pk; }

    bool isValid() const { return deviceName.isNotEmpty() && hostAddress.isNotEmpty(); }

private:
    juce::String deviceName;
    juce::String hostAddress;
    int port = 7000;
    juce::String deviceId;
    juce::String password;
    bool needsPassword = false;
    juce::String serverPublicKey; // RAOP server RSA public key (TXT record 'pk')
};
