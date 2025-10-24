#include "DeviceDiscovery.h"

DeviceDiscovery::DeviceDiscovery()
{
    createPlatformImpl();
}

DeviceDiscovery::~DeviceDiscovery()
{
    stopDiscovery();
    destroyPlatformImpl();
}

// startDiscovery and stopDiscovery are implemented in DeviceDiscoveryMac.mm

void DeviceDiscovery::addListener(AirPlayPluginEditor* listener)
{
    listeners.addIfNotAlreadyThere(listener);
}

void DeviceDiscovery::removeListener(AirPlayPluginEditor* listener)
{
    listeners.removeAllInstancesOf(listener);
}

juce::Array<AirPlayDevice> DeviceDiscovery::getDiscoveredDevices() const
{
    const juce::ScopedLock sl(deviceLock);
    return discoveredDevices;
}

void DeviceDiscovery::addDiscoveredDevice(const AirPlayDevice& device)
{
    {
        const juce::ScopedLock sl(deviceLock);
        discoveredDevices.addIfNotAlreadyThere(device);
    }

    // Notify via callback if set
    if (deviceFoundCallback)
    {
        deviceFoundCallback(device);
    }
}

void DeviceDiscovery::setDeviceFoundCallback(DeviceFoundCallback callback)
{
    const juce::ScopedLock sl(deviceLock);
    deviceFoundCallback = callback;
}

void DeviceDiscovery::setDeviceLostCallback(DeviceLostCallback callback)
{
    const juce::ScopedLock sl(deviceLock);
    deviceLostCallback = callback;
}

// Platform-specific implementations are in DeviceDiscoveryMac.mm
