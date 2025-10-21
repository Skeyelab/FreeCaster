#if JUCE_WINDOWS

#include "AirPlayWindows.h"

AirPlayWindows::AirPlayWindows()
{
    raopClient = std::make_unique<RaopClient>();
    encoder = std::make_unique<AudioEncoder>();
    encoder->setFormat(AudioEncoder::Format::PCM_16);
}

AirPlayWindows::~AirPlayWindows()
{
    disconnect();
}

bool AirPlayWindows::connect(const AirPlayDevice& device)
{
    return raopClient->connect(device);
}

void AirPlayWindows::disconnect()
{
    raopClient->disconnect();
}

bool AirPlayWindows::isConnected() const
{
    return raopClient->isConnected();
}

bool AirPlayWindows::streamAudio(const juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (!isConnected())
        return false;
    
    auto encodedData = encoder->encode(buffer, numSamples);
    return raopClient->sendAudio(encodedData, 44100, buffer.getNumChannels());
}

juce::String AirPlayWindows::getLastError() const
{
    return raopClient->getLastError();
}

#endif
