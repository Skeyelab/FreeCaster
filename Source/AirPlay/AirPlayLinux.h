#pragma once

#if JUCE_LINUX

#include "AirPlayManager.h"
#include "RaopClient.h"

class AirPlayLinux : public AirPlayStreamer
{
public:
    AirPlayLinux();
    ~AirPlayLinux() override;
    
    bool connect(const AirPlayDevice& device) override;
    void disconnect() override;
    bool isConnected() const override;
    bool streamAudio(const juce::AudioBuffer<float>& buffer, int numSamples) override;
    juce::String getLastError() const override;
    
private:
    std::unique_ptr<RaopClient> raopClient;
    std::unique_ptr<AudioEncoder> encoder;
};

#endif
