#pragma once
#include <JuceHeader.h>
#include "AirPlayDevice.h"

// Forward declaration
class AirPlayPluginEditor;

class DeviceDiscovery
{
public:
    // Callback function type for device discovery events
    using DeviceFoundCallback = std::function<void(const AirPlayDevice&)>;
    using DeviceLostCallback = std::function<void(const AirPlayDevice&)>;

    DeviceDiscovery();
    ~DeviceDiscovery();

    void startDiscovery();
    void stopDiscovery();

    void addListener(AirPlayPluginEditor* listener);
    void removeListener(AirPlayPluginEditor* listener);

    // Alternative callback-based approach
    void setDeviceFoundCallback(DeviceFoundCallback callback);
    void setDeviceLostCallback(DeviceLostCallback callback);

    juce::Array<AirPlayDevice> getDiscoveredDevices() const;
    void addDiscoveredDevice(const AirPlayDevice& device);

private:
    juce::Array<AirPlayPluginEditor*> listeners;
    juce::Array<AirPlayDevice> discoveredDevices;
    juce::CriticalSection deviceLock;

    bool isDiscovering = false;

    // Callback functions
    DeviceFoundCallback deviceFoundCallback;
    DeviceLostCallback deviceLostCallback;

    // Platform-specific implementation
    void* platformImpl = nullptr;

    void createPlatformImpl();
    void destroyPlatformImpl();
};
