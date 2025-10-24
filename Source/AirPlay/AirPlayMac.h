#pragma once

#include <JuceHeader.h>
#include "../Discovery/AirPlayDevice.h"

#if JUCE_MAC || JUCE_IOS

class AirPlayMac
{
public:
    AirPlayMac();
    ~AirPlayMac();

    bool connect(const AirPlayDevice& device);
    void disconnect();
    bool isConnected() const;
    bool streamAudio(const juce::AudioBuffer<float>& buffer, int numSamples);
    juce::String getLastError() const;

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;

    bool connected = false;
    juce::String lastError;
};

#endif
