#include "DeviceDiscovery.h"
#include "DeviceDiscoveryPlatform.h"

DeviceDiscovery::DeviceDiscovery() : Thread("AirPlayDiscovery")
{
#if JUCE_MAC
    platformImpl = std::make_unique<PlatformImpl>(this);
#endif
}

DeviceDiscovery::~DeviceDiscovery()
{
    stopDiscovery();
    platformImpl.reset();
}

void DeviceDiscovery::startDiscovery()
{
#if JUCE_MAC
    if (platformImpl)
        platformImpl->start();
#endif

    if (!isThreadRunning())
        startThread();
}

void DeviceDiscovery::stopDiscovery()
{
#if JUCE_MAC
    if (platformImpl)
        platformImpl->stop();
#endif

    signalThreadShouldExit();
    notify();
    stopThread(2000);
}

void DeviceDiscovery::addListener(Listener* listener)
{
    listeners.add(listener);
}

void DeviceDiscovery::removeListener(Listener* listener)
{
    listeners.remove(listener);
}

juce::Array<AirPlayDevice> DeviceDiscovery::getDiscoveredDevices() const
{
    const juce::ScopedLock sl(deviceLock);
    return devices;
}

void DeviceDiscovery::run()
{
    while (!threadShouldExit())
    {
        performDiscovery();
        wait(5000);
    }
}

void DeviceDiscovery::performDiscovery()
{
    // Platform-specific mDNS discovery runs in PlatformImpl
    // This thread just keeps running for future cross-platform polling if needed
}

void DeviceDiscovery::addDiscoveredDevice(const AirPlayDevice& device)
{
    const juce::ScopedLock sl(deviceLock);

    // Check if device already exists
    bool found = false;
    for (const auto& existing : devices)
    {
        if (existing.getDeviceName() == device.getDeviceName())
        {
            found = true;
            break;
        }
    }

    if (!found)
    {
        devices.add(device);
        listeners.call([&](Listener& l) { l.deviceFound(device); });
    }
}
