#include "AirPlayManager.h"
#include "AirPlayMac.h"

AirPlayManager::AirPlayManager() : Thread("AirPlayStream")
{
    encoder = std::make_unique<AudioEncoder>();
    buffer = std::make_unique<StreamBuffer>(2, 8192);
    airplayImpl = std::make_unique<AirPlayMac>();
}

AirPlayManager::~AirPlayManager()
{
    // Clear callbacks first to prevent UI access during destruction
    clearCallbacks();
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
    juce::Logger::writeToLog("AirPlayManager: Connecting to device: " + device.getDeviceName());
    const juce::ScopedLock sl(connectionLock);

    hasError = false;
    isReconnecting = false;

    if (airplayImpl && airplayImpl->connect(device))
    {
        connectedDevice = device;
        notifyStatusChange("Connected to: " + device.getDeviceName());

        if (!isThreadRunning())
            startThread();
    }
    else
    {
        lastError = airplayImpl ? airplayImpl->getLastError() : "Failed to create AirPlay implementation";
        notifyError(lastError);
        hasError = true;
    }
}

void AirPlayManager::disconnectFromDevice()
{
    juce::Logger::writeToLog("AirPlayManager: Disconnecting");
    const juce::ScopedLock sl(connectionLock);

    if (airplayImpl)
    {
        airplayImpl->disconnect();
    }

    // Only notify if we have a valid callback (UI still exists)
    if (onStatusChange)
    {
        notifyStatusChange("Disconnected");
    }
}

bool AirPlayManager::isConnected() const
{
    const juce::ScopedLock sl(connectionLock);
    return airplayImpl && airplayImpl->isConnected();
}

juce::String AirPlayManager::getConnectedDeviceName() const
{
    const juce::ScopedLock sl(connectionLock);
    return isConnected() ? connectedDevice.getDeviceName() : "";
}

juce::String AirPlayManager::getConnectionStatus() const
{
    if (!isConnected())
        return "Disconnected";

    return "Connected to: " + getConnectedDeviceName();
}

void AirPlayManager::pushAudioData(const juce::AudioBuffer<float>& audioBuffer, int numSamples)
{
    if (!isConnected())
        return;

    buffer->write(audioBuffer, numSamples);
}

juce::String AirPlayManager::getLastError() const
{
    return lastError;
}

void AirPlayManager::setAutoReconnect(bool /*enable*/)
{
    // Auto-reconnect handled by macOS framework
}

bool AirPlayManager::isAutoReconnectEnabled() const
{
    return true; // Always enabled on macOS
}

void AirPlayManager::run()
{
    while (!threadShouldExit())
    {
        processAudioStream();
        monitorConnection();
        juce::Thread::sleep(10);
    }
}

void AirPlayManager::processAudioStream()
{
    const juce::ScopedLock sl(connectionLock);

    if (!isConnected() || !airplayImpl)
        return;

    juce::AudioBuffer<float> audioBuffer(2, currentSamplesPerBlock);
    int samplesRead = buffer->read(audioBuffer, currentSamplesPerBlock);

    if (samplesRead > 0)
    {
        if (!airplayImpl->streamAudio(audioBuffer, samplesRead))
        {
            notifyError("Failed to stream audio");
            hasError = true;
        }
    }
}

void AirPlayManager::monitorConnection()
{
    if (hasError && !isReconnecting)
    {
        isReconnecting = true;
        // macOS handles reconnection automatically through AVFoundation
    }
}

void AirPlayManager::notifyError(const juce::String& error)
{
    if (onError)
        onError(error);
}

void AirPlayManager::notifyStatusChange(const juce::String& status)
{
    if (onStatusChange)
        onStatusChange(status);
}

void AirPlayManager::clearCallbacks()
{
    onError = nullptr;
    onStatusChange = nullptr;
}
