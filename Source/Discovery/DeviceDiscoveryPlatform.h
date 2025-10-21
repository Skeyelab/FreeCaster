#pragma once

class DeviceDiscovery;

#if JUCE_MAC

struct DeviceDiscovery::PlatformImpl
{
    void* browser;  // AirPlayServiceBrowser* (Obj-C)
    
    PlatformImpl(DeviceDiscovery* owner);
    ~PlatformImpl();
    
    void start();
    void stop();
};

#else

// Placeholder for Windows/Linux
struct DeviceDiscovery::PlatformImpl
{
    PlatformImpl(DeviceDiscovery*) {}
    ~PlatformImpl() {}
    void start() {}
    void stop() {}
};

#endif
