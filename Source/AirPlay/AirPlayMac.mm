#include "AirPlayMac.h"

#if JUCE_MAC

// RaopClient removed - using native macOS AirPlay
#include "../Audio/AudioEncoder.h"

struct AirPlayMac::Impl
{
    std::unique_ptr<AudioEncoder> encoder;
    bool isConnected = false;
    juce::String lastError;
};

AirPlayMac::AirPlayMac()
{
    pimpl = std::make_unique<Impl>();
    pimpl->encoder = std::make_unique<AudioEncoder>();
    pimpl->encoder->setFormat(AudioEncoder::Format::PCM_16);
}

AirPlayMac::~AirPlayMac()
{
    disconnect();
}

bool AirPlayMac::connect(const AirPlayDevice& device)
{
    DBG("Connecting to AirPlay device: " + device.getDeviceName() + " at " + device.getHostAddress() + ":" + juce::String(device.getPort()));

    // TODO: Implement native macOS AirPlay connection
    // For now, just simulate a successful connection
    pimpl->isConnected = true;
    pimpl->lastError = "";
    connected = true;
    lastError = "";
    DBG("Successfully connected to " + device.getDeviceName() + " (stub implementation)");
    return true;
}

void AirPlayMac::disconnect()
{
    if (!connected)
        return;

    DBG("Disconnecting from AirPlay device");

    // TODO: Implement native macOS AirPlay disconnection
    pimpl->isConnected = false;
    connected = false;
}

bool AirPlayMac::isConnected() const
{
    return connected;
}

bool AirPlayMac::streamAudio(const juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (!connected || !pimpl->encoder)
        return false;

    // TODO: Implement native macOS AirPlay streaming
    // For now, just encode the audio (stub implementation)
    auto encodedData = pimpl->encoder->encode(buffer, numSamples);

    // Simulate successful streaming
    return true;
}

juce::String AirPlayMac::getLastError() const
{
    return lastError;
}

#endif
