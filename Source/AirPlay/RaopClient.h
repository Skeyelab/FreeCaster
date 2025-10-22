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
    // RTP header structure (12 bytes)
    struct RTPHeader
    {
        uint8_t version_flags;     // Version (2), Padding (1), Extension (1), CSRC Count (4)
        uint8_t payload_type;      // Payload type (7), Marker (1)
        uint16_t sequence_number;  // Sequence number
        uint32_t timestamp;        // RTP timestamp
        uint32_t ssrc;            // Synchronization source identifier
    };

    // NTP timestamp structure for synchronization
    struct NTPTimestamp
    {
        uint32_t seconds;
        uint32_t fraction;
    };

    bool sendRtspRequest(const juce::String& method, const juce::String& uri, const juce::StringPairArray& headers, juce::StringPairArray* responseHeaders = nullptr);
    bool sendSetup();
    bool sendRecord();
    bool sendTeardown();

    bool createUdpSockets();
    void closeUdpSockets();
    bool sendRtpPacket(const void* data, size_t size);
    NTPTimestamp getCurrentNtpTimestamp() const;

    std::unique_ptr<juce::StreamingSocket> socket;
    std::unique_ptr<juce::DatagramSocket> audioSocket;
    std::unique_ptr<juce::DatagramSocket> controlSocket;
    std::unique_ptr<juce::DatagramSocket> timingSocket;

    AirPlayDevice currentDevice;
    bool connected = false;
    juce::String lastError;

    // RTSP session info
    int serverPort = 0;
    int controlPort = 0;
    int timingPort = 0;
    juce::String session;

    // RTP state
    uint16_t sequenceNumber = 0;
    uint32_t rtpTimestamp = 0;
    uint32_t ssrc = 0x12345678;  // Random SSRC
    juce::Time startTime;        // For NTP timestamp calculation

    // UDP ports
    int clientAudioPort = 6000;
    int clientControlPort = 6001;
    int clientTimingPort = 6002;
};
