#pragma once

#include "AirPlayManager.h"

#if JUCE_MAC || JUCE_IOS

class AirPlayMac : public AirPlayStreamer
{
public:
    AirPlayMac();
    ~AirPlayMac() override;

    bool connect(const AirPlayDevice& device) override;
    void disconnect() override;
    bool isConnected() const override;
    bool streamAudio(const juce::AudioBuffer<float>& buffer, int numSamples) override;
    juce::String getLastError() const override;

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;

    bool connected = false;
    juce::String lastError;
};

#endif
