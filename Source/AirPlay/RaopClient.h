#pragma once
#include <JuceHeader.h>
#include "../Discovery/AirPlayDevice.h"

class RaopClient
{
public:
    RaopClient();
    ~RaopClient();
    
    bool connect(const AirPlayDevice& device);
    void disconnect();
    bool isConnected() const;
    
    bool sendAudio(const juce::MemoryBlock& audioData, int sampleRate, int channels);
    
    juce::String getLastError() const { return lastError; }
    
private:
    bool sendRtspRequest(const juce::String& method, const juce::String& uri, const juce::StringPairArray& headers);
    bool sendSetup();
    bool sendRecord();
    bool sendTeardown();
    
    std::unique_ptr<juce::StreamingSocket> socket;
    AirPlayDevice currentDevice;
    bool connected = false;
    juce::String lastError;
    
    int serverPort = 0;
    int controlPort = 0;
    int timingPort = 0;
    juce::String session;
};
