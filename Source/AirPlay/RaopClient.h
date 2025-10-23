#pragma once
#include <JuceHeader.h>
#include "../Discovery/AirPlayDevice.h"
#include "AirPlayAuth.h"

class RaopClient
{
public:
    // Connection states
    enum class ConnectionState
    {
        Disconnected,
        Connecting,
        Connected,
        Reconnecting,
        Error,
        TimedOut
    };

    RaopClient();
    ~RaopClient();

    bool connect(const AirPlayDevice& device);
    void disconnect();
    bool isConnected() const;
    ConnectionState getConnectionState() const { return connectionState; }
    juce::String getConnectionStateString() const;

    bool sendAudio(const juce::MemoryBlock& audioData, int sampleRate, int channels);

    juce::String getLastError() const { return lastError; }
    
    // Auto-reconnect settings
    void setAutoReconnect(bool enable) { autoReconnectEnabled = enable; }
    bool isAutoReconnectEnabled() const { return autoReconnectEnabled; }
    
    // Connection health monitoring
    bool checkConnection();
    juce::int64 getLastSuccessfulSendTime() const { return lastSuccessfulSendTime; }
    int getConsecutiveFailures() const { return consecutiveFailures; }

    // Authentication support
    void setPassword(const juce::String& password);
    bool requiresAuthentication() const { return useAuthentication; }
    void setUseAuthentication(bool enable) { useAuthentication = enable; }

    // Public for testing
    struct RtspResponse
    {
        int statusCode = 0;
        juce::String statusMessage;
        juce::StringPairArray headers;
        juce::String body;
        
        bool isSuccess() const { return statusCode >= 200 && statusCode < 300; }
    };
    
    bool parseRtspResponse(const juce::String& responseText, RtspResponse& response);
    bool parseTransportHeader(const juce::String& transport, int& audioPort, int& controlPort, int& timingPort);

private:
    // Connection management
    void setConnectionState(ConnectionState newState);
    bool attemptReconnect();
    void logError(const juce::String& error);
    bool waitForSocketReady(juce::StreamingSocket* sock, int timeoutMs);
    
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

    bool sendRtspRequest(const juce::String& method, const juce::String& uri, const juce::StringPairArray& headers, RtspResponse* response = nullptr);
    bool sendRtspRequest(const juce::String& method, const juce::String& uri, const juce::StringPairArray& headers, const juce::String& body, RtspResponse* response = nullptr);
    bool sendOptions();
    bool sendAnnounce();
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
    ConnectionState connectionState = ConnectionState::Disconnected;
    juce::String lastError;
    
    // Reliability and monitoring
    bool autoReconnectEnabled = true;
    int reconnectAttempts = 0;
    static constexpr int maxReconnectAttempts = 5;
    juce::int64 lastSuccessfulSendTime = 0;
    juce::int64 lastConnectionAttemptTime = 0;
    int consecutiveFailures = 0;
    static constexpr int maxConsecutiveFailures = 10;
    
    // Thread safety
    juce::CriticalSection stateLock;

    // Authentication
    std::unique_ptr<AirPlayAuth> auth;
    bool useAuthentication = true;  // Enable by default
    int cseq = 1;  // RTSP sequence number

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
