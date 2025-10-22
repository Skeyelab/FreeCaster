#include "AirPlayLinux.h"

AirPlayLinux::AirPlayLinux()
{
    raopClient = std::make_unique<RaopClient>();
    encoder = std::make_unique<AudioEncoder>();
    encoder->setFormat(AudioEncoder::Format::PCM_16);
}

AirPlayLinux::~AirPlayLinux()
{
    disconnect();
}

bool AirPlayLinux::connect(const AirPlayDevice& device)
{
    return raopClient->connect(device);
}

void AirPlayLinux::disconnect()
{
    raopClient->disconnect();
}

bool AirPlayLinux::isConnected() const
{
    return raopClient->isConnected();
}

bool AirPlayLinux::streamAudio(const juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (!isConnected())
        return false;
    
    auto encodedData = encoder->encode(buffer, numSamples);
    return raopClient->sendAudio(encodedData, 44100, buffer.getNumChannels());
}

juce::String AirPlayLinux::getLastError() const
{
    return raopClient->getLastError();
}
