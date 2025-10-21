#include "AirPlayMac.h"

#if JUCE_MAC

#include "RaopClient.h"
#include "../Audio/AudioEncoder.h"

struct AirPlayMac::Impl
{
    std::unique_ptr<RaopClient> raopClient;
    std::unique_ptr<AudioEncoder> encoder;
};

AirPlayMac::AirPlayMac()
{
    pimpl = std::make_unique<Impl>();
    pimpl->raopClient = std::make_unique<RaopClient>();
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

    if (!pimpl->raopClient)
    {
        lastError = "RAOP client not initialized";
        DBG("ERROR: " + lastError);
        return false;
    }

    if (pimpl->raopClient->connect(device))
    {
        connected = true;
        lastError = "";
        DBG("Successfully connected to " + device.getDeviceName());
        return true;
    }
    else
    {
        connected = false;
        lastError = pimpl->raopClient->getLastError();
        DBG("Connection failed: " + lastError);
        return false;
    }
}

void AirPlayMac::disconnect()
{
    if (!connected)
        return;

    DBG("Disconnecting from AirPlay device");

    if (pimpl->raopClient)
    {
        pimpl->raopClient->disconnect();
    }

    connected = false;
}

bool AirPlayMac::isConnected() const
{
    return connected;
}

bool AirPlayMac::streamAudio(const juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (!connected || !pimpl->raopClient || !pimpl->encoder)
        return false;

    // Encode audio to PCM16
    auto encodedData = pimpl->encoder->encode(buffer, numSamples);

    // Send to AirPlay device via RAOP
    return pimpl->raopClient->sendAudio(encodedData, 44100, buffer.getNumChannels());
}

juce::String AirPlayMac::getLastError() const
{
    return lastError;
}

#endif
