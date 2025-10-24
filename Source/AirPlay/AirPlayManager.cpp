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
    juce::Logger::writeToLog("AirPlayManager: connectToDevice called for: " + device.getDeviceName() + " at " + device.getHostAddress() + ":" + juce::String(device.getPort()));
    const juce::ScopedLock sl(connectionLock);

    hasError = false;
    isReconnecting = false;

    if (streamer)
    {
        juce::Logger::writeToLog("AirPlayManager: Streamer exists, attempting connection");
        if (streamer->connect(device))
        {
            connectedDevice = device;
            lastError.clear();
            startThread();
            notifyStatusChange("Connected to " + device.getDeviceName());
            juce::Logger::writeToLog("AirPlayManager: Connected to " + device.getDeviceName());
        }
        else
        {
            lastError = streamer->getLastError();
            hasError = true;
            notifyError("Connection failed: " + lastError);
            juce::Logger::writeToLog("AirPlayManager: Connection failed - " + lastError);
        }
    }
    else
    {
        lastError = "Streamer not available";
        hasError = true;
        notifyError("Connection failed: " + lastError);
        juce::Logger::writeToLog("AirPlayManager: Streamer not available");
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
    hasError = false;
    isReconnecting = false;
    notifyStatusChange("Disconnected");
    juce::Logger::writeToLog("AirPlayManager: Disconnected");
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
    const juce::ScopedLock sl(connectionLock);
    return lastError;
}

juce::String AirPlayManager::getConnectionStatus() const
{
    const juce::ScopedLock sl(connectionLock);

    if (!streamer)
        return "Not initialized";

    if (isReconnecting)
        return "Reconnecting...";

    if (hasError)
        return "Error: " + lastError;

    if (streamer->isConnected())
        return "Connected to " + connectedDevice.getDeviceName();

    return "Disconnected";
}

void AirPlayManager::setAutoReconnect(bool enable)
{
    if (streamer)
    {
        // Platform-specific implementation would need this method
        // For now, just log it
        juce::Logger::writeToLog("AirPlayManager: Auto-reconnect " + juce::String(enable ? "enabled" : "disabled"));
    }
}

bool AirPlayManager::isAutoReconnectEnabled() const
{
    // Would return platform-specific streamer's auto-reconnect status
    return true;  // Default enabled
}

void AirPlayManager::run()
{
    while (!threadShouldExit())
    {
        processAudioStream();
        monitorConnection();
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
        if (!streamer->streamAudio(tempBuffer, samplesRead))
        {
            // Stream failed
            const juce::ScopedLock sl(connectionLock);
            lastError = streamer->getLastError();
            hasError = true;
            notifyError("Audio streaming error: " + lastError);
        }
    }
}

void AirPlayManager::monitorConnection()
{
    juce::int64 now = juce::Time::currentTimeMillis();

    // Monitor every 5 seconds
    if (now - lastMonitorTime < 5000)
        return;

    lastMonitorTime = now;

    if (!streamer)
        return;

    // Check if we're still connected
    if (streamer->isConnected())
    {
        // Connection is healthy
        if (hasError)
        {
            // Recovered from error
            hasError = false;
            isReconnecting = false;
            notifyStatusChange("Connection recovered");
            juce::Logger::writeToLog("AirPlayManager: Connection recovered");
        }
    }
    else if (!hasError)
    {
        // Connection lost
        const juce::ScopedLock sl(connectionLock);
        lastError = "Connection lost to " + connectedDevice.getDeviceName();
        hasError = true;
        notifyError(lastError);
        juce::Logger::writeToLog("AirPlayManager: " + lastError);
    }
}

void AirPlayManager::notifyError(const juce::String& error)
{
    if (onError)
    {
        juce::MessageManager::callAsync([this, error]()
        {
            if (onError)
                onError(error);
        });
    }
}

void AirPlayManager::notifyStatusChange(const juce::String& status)
{
    if (onStatusChange)
    {
        juce::MessageManager::callAsync([this, status]()
        {
            if (onStatusChange)
                onStatusChange(status);
        });
    }
}
