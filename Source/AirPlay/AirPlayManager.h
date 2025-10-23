#pragma once
#include <JuceHeader.h>
#include "../Discovery/AirPlayDevice.h"
#include "../Audio/AudioEncoder.h"
#include "../Audio/StreamBuffer.h"

class AirPlayStreamer
{
public:
    virtual ~AirPlayStreamer() = default;
    virtual bool connect(const AirPlayDevice& device) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual bool streamAudio(const juce::AudioBuffer<float>& buffer, int numSamples) = 0;
    virtual juce::String getLastError() const = 0;
};

class AirPlayManager : public juce::Thread
{
public:
    AirPlayManager();
    ~AirPlayManager() override;
    
    void prepare(double sampleRate, int samplesPerBlock);
    void connectToDevice(const AirPlayDevice& device);
    void disconnectFromDevice();
    
    bool isConnected() const;
    juce::String getConnectedDeviceName() const;
    juce::String getConnectionStatus() const;
    
    void pushAudioData(const juce::AudioBuffer<float>& buffer, int numSamples);
    
    juce::String getLastError() const;
    
    // Auto-reconnect settings
    void setAutoReconnect(bool enable);
    bool isAutoReconnectEnabled() const;
    
    // Error callback for GUI updates
    std::function<void(const juce::String&)> onError;
    std::function<void(const juce::String&)> onStatusChange;
    
private:
    void run() override;
    void processAudioStream();
    void monitorConnection();
    void notifyError(const juce::String& error);
    void notifyStatusChange(const juce::String& status);
    
    std::unique_ptr<AirPlayStreamer> streamer;
    std::unique_ptr<AudioEncoder> encoder;
    std::unique_ptr<StreamBuffer> buffer;
    
    AirPlayDevice connectedDevice;
    double currentSampleRate = 44100.0;
    int currentSamplesPerBlock = 512;
    
    juce::CriticalSection connectionLock;
    juce::String lastError;
    juce::int64 lastMonitorTime = 0;
    std::atomic<bool> hasError{false};
    std::atomic<bool> isReconnecting{false};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AirPlayManager)
};
