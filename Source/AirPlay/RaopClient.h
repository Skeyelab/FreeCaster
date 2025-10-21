#pragma once
#include <JuceHeader.h>
#include "../Discovery/AirPlayDevice.h"
#include <cstdint>
#include <memory>

// RTP packet header structure (RFC 3550)
#pragma pack(push, 1)
struct RtpHeader
{
    // Byte 0
    uint8_t csrcCount : 4;   // CSRC count
    uint8_t extension : 1;    // Extension bit
    uint8_t padding : 1;      // Padding bit
    uint8_t version : 2;      // Version (always 2)
    
    // Byte 1
    uint8_t payloadType : 7;  // Payload type
    uint8_t marker : 1;       // Marker bit
    
    // Bytes 2-3
    uint16_t sequenceNumber;  // Sequence number
    
    // Bytes 4-7
    uint32_t timestamp;       // Timestamp
    
    // Bytes 8-11
    uint32_t ssrc;            // Synchronization source identifier
};
#pragma pack(pop)

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
    bool receiveRtspResponse(juce::StringPairArray& headers, juce::String& body);
    bool sendSetup();
    bool sendRecord();
    bool sendTeardown();
    
    // RTP packet construction
    void buildRtpPacket(juce::MemoryBlock& packet, const uint8_t* audioData, size_t audioSize);
    uint32_t getCurrentRtpTimestamp(int sampleRate);
    uint64_t getNtpTimestamp();
    
    // Network sockets
    std::unique_ptr<juce::StreamingSocket> rtspSocket;  // RTSP control socket (TCP)
    std::unique_ptr<juce::DatagramSocket> audioSocket;  // RTP audio socket (UDP)
    std::unique_ptr<juce::DatagramSocket> controlSocket; // RTCP control socket (UDP)
    std::unique_ptr<juce::DatagramSocket> timingSocket;  // Timing socket (UDP)
    
    AirPlayDevice currentDevice;
    bool connected = false;
    juce::String lastError;
    
    // Ports negotiated during RTSP handshake
    int serverAudioPort = 0;
    int serverControlPort = 0;
    int serverTimingPort = 0;
    int localAudioPort = 0;
    int localControlPort = 0;
    int localTimingPort = 0;
    
    juce::String sessionId;
    
    // RTP state
    uint16_t sequenceNumber = 0;
    uint32_t ssrc;
    uint64_t rtpStartTime = 0;
    int64_t samplesTransmitted = 0;
    
    // Sequence counter for RTSP
    int cseq = 1;
};
