#pragma once

#include <memory>

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

#elif JUCE_WINDOWS || JUCE_LINUX

// Windows/Linux implementation
struct DeviceDiscovery::PlatformImpl
{
    void* impl;
    
    PlatformImpl(DeviceDiscovery* owner);
    ~PlatformImpl();
    
    void start();
    void stop();
};

#else

// Fallback for unsupported platforms
struct DeviceDiscovery::PlatformImpl
{
    PlatformImpl(DeviceDiscovery*) {}
    ~PlatformImpl() {}
    void start() {}
    void stop() {}
};

#endif
