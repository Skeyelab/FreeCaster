#pragma once
#include <JuceHeader.h>
#include "AirPlayDevice.h"

class DeviceDiscovery : public juce::Thread
{
public:
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void deviceFound(const AirPlayDevice& device) = 0;
        virtual void deviceLost(const AirPlayDevice& device) = 0;
    };
    
    DeviceDiscovery();
    ~DeviceDiscovery() override;
    
    void startDiscovery();
    void stopDiscovery();
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    juce::Array<AirPlayDevice> getDiscoveredDevices() const;
    
    void addDiscoveredDevice(const AirPlayDevice& device);
    
private:
    void run() override;
    void performDiscovery();
    
    juce::ListenerList<Listener> listeners;
    juce::Array<AirPlayDevice> devices;
    juce::CriticalSection deviceLock;
    
    struct PlatformImpl;
    std::unique_ptr<PlatformImpl> platformImpl;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeviceDiscovery)
};
