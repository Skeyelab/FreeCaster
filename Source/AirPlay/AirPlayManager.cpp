#include "AirPlayManager.h"

#if JUCE_MAC
    #include "AirPlayMac.h"
#elif JUCE_WINDOWS
    #include "AirPlayWindows.h"
#elif JUCE_LINUX
    #include "AirPlayLinux.h"
#endif

AirPlayManager::AirPlayManager() : Thread("AirPlayStream")
{
    encoder = std::make_unique<AudioEncoder>();
    buffer = std::make_unique<StreamBuffer>(2, 8192);
    
#if JUCE_MAC
    streamer = std::make_unique<AirPlayMac>();
#elif JUCE_WINDOWS
    streamer = std::make_unique<AirPlayWindows>();
#elif JUCE_LINUX
    streamer = std::make_unique<AirPlayLinux>();
#endif
}

AirPlayManager::~AirPlayManager()
{
    disconnectFromDevice();
    stopThread(2000);
}

void AirPlayManager::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentSamplesPerBlock = samplesPerBlock;
    encoder->prepare(sampleRate, samplesPerBlock);
}

void AirPlayManager::connectToDevice(const AirPlayDevice& device)
{
    const juce::ScopedLock sl(connectionLock);
    
    if (streamer && streamer->connect(device))
    {
        connectedDevice = device;
        startThread();
    }
}

void AirPlayManager::disconnectFromDevice()
{
    const juce::ScopedLock sl(connectionLock);
    
    signalThreadShouldExit();
    notify();
    stopThread(2000);
    
    if (streamer)
        streamer->disconnect();
    
    connectedDevice = AirPlayDevice();
}

bool AirPlayManager::isConnected() const
{
    const juce::ScopedLock sl(connectionLock);
    return streamer && streamer->isConnected();
}

juce::String AirPlayManager::getConnectedDeviceName() const
{
    const juce::ScopedLock sl(connectionLock);
    return connectedDevice.getDeviceName();
}

void AirPlayManager::pushAudioData(const juce::AudioBuffer<float>& audioBuffer, int numSamples)
{
    if (buffer)
        buffer->write(audioBuffer, numSamples);
}

juce::String AirPlayManager::getLastError() const
{
    if (streamer)
        return streamer->getLastError();
    return "No streamer available";
}

void AirPlayManager::run()
{
    while (!threadShouldExit())
    {
        processAudioStream();
        wait(10);
    }
}

void AirPlayManager::processAudioStream()
{
    if (!isConnected() || !buffer || !encoder || !streamer)
        return;
    
    juce::AudioBuffer<float> tempBuffer(2, currentSamplesPerBlock);
    int samplesRead = buffer->read(tempBuffer, currentSamplesPerBlock);
    
    if (samplesRead > 0)
    {
        streamer->streamAudio(tempBuffer, samplesRead);
    }
}
