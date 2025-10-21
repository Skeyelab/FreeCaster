#include "RaopClient.h"
#include <random>
#include <chrono>

RaopClient::RaopClient()
{
    rtspSocket = std::make_unique<juce::StreamingSocket>();
    audioSocket = std::make_unique<juce::DatagramSocket>();
    controlSocket = std::make_unique<juce::DatagramSocket>();
    timingSocket = std::make_unique<juce::DatagramSocket>();
    
    // Generate random SSRC (synchronization source identifier)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dist(0, UINT32_MAX);
    ssrc = dist(gen);
    
    // Initialize sequence number randomly
    sequenceNumber = static_cast<uint16_t>(dist(gen) & 0xFFFF);
}

RaopClient::~RaopClient()
{
    disconnect();
}

bool RaopClient::connect(const AirPlayDevice& device)
{
    if (connected)
        disconnect();
    
    currentDevice = device;
    
    // Connect RTSP socket
    if (!rtspSocket->connect(device.getHostAddress(), device.getPort(), 5000))
    {
        lastError = "Failed to connect to " + device.getHostAddress();
        return false;
    }
    
    // Bind UDP sockets to random local ports
    if (!audioSocket->bindToPort(0))  // 0 = let OS choose port
    {
        lastError = "Failed to bind audio UDP socket";
        return false;
    }
    localAudioPort = audioSocket->getBoundPort();
    
    if (!controlSocket->bindToPort(0))
    {
        lastError = "Failed to bind control UDP socket";
        return false;
    }
    localControlPort = controlSocket->getBoundPort();
    
    if (!timingSocket->bindToPort(0))
    {
        lastError = "Failed to bind timing UDP socket";
        return false;
    }
    localTimingPort = timingSocket->getBoundPort();
    
    if (!sendSetup())
    {
        disconnect();
        return false;
    }
    
    if (!sendRecord())
    {
        disconnect();
        return false;
    }
    
    connected = true;
    rtpStartTime = juce::Time::currentTimeMillis();
    samplesTransmitted = 0;
    
    return true;
}

void RaopClient::disconnect()
{
    if (connected)
    {
        sendTeardown();
        rtspSocket->close();
        connected = false;
    }
    
    // Close all UDP sockets
    audioSocket->shutdown();
    controlSocket->shutdown();
    timingSocket->shutdown();
}

bool RaopClient::isConnected() const
{
    return connected;
}

bool RaopClient::sendAudio(const juce::MemoryBlock& audioData, int sampleRate, int channels)
{
    if (!connected || serverAudioPort == 0)
        return false;
    
    // RTP payload type 96 is typically used for dynamic payload (L16 audio)
    const int RTP_PAYLOAD_TYPE = 96;
    const size_t MAX_PAYLOAD_SIZE = 1408; // Safe MTU size minus headers
    
    const uint8_t* audioPtr = static_cast<const uint8_t*>(audioData.getData());
    size_t remainingBytes = audioData.getSize();
    
    while (remainingBytes > 0)
    {
        size_t payloadSize = std::min(remainingBytes, MAX_PAYLOAD_SIZE);
        
        // Build RTP packet
        juce::MemoryBlock packet;
        buildRtpPacket(packet, audioPtr, payloadSize);
        
        // Send UDP packet to server
        int bytesSent = audioSocket->write(
            currentDevice.getHostAddress(),
            serverAudioPort,
            packet.getData(),
            static_cast<int>(packet.getSize())
        );
        
        if (bytesSent < 0)
        {
            lastError = "Failed to send RTP packet";
            return false;
        }
        
        audioPtr += payloadSize;
        remainingBytes -= payloadSize;
        
        // Update state for next packet
        sequenceNumber++;
        samplesTransmitted += (payloadSize / (channels * 2)); // 2 bytes per sample for 16-bit
    }
    
    return true;
}

void RaopClient::buildRtpPacket(juce::MemoryBlock& packet, const uint8_t* audioData, size_t audioSize)
{
    const size_t headerSize = sizeof(RtpHeader);
    packet.setSize(headerSize + audioSize);
    
    RtpHeader* header = static_cast<RtpHeader*>(packet.getData());
    
    // Fill RTP header
    header->version = 2;
    header->padding = 0;
    header->extension = 0;
    header->csrcCount = 0;
    header->marker = 0;
    header->payloadType = 96; // Dynamic payload type for L16
    
    // Network byte order (big-endian)
    header->sequenceNumber = juce::ByteOrder::swapIfLittleEndian(sequenceNumber);
    header->timestamp = juce::ByteOrder::swapIfLittleEndian(static_cast<uint32_t>(samplesTransmitted));
    header->ssrc = juce::ByteOrder::swapIfLittleEndian(ssrc);
    
    // Copy audio payload after header
    memcpy(static_cast<uint8_t*>(packet.getData()) + headerSize, audioData, audioSize);
}

uint32_t RaopClient::getCurrentRtpTimestamp(int sampleRate)
{
    uint64_t elapsed = juce::Time::currentTimeMillis() - rtpStartTime;
    return static_cast<uint32_t>((elapsed * sampleRate) / 1000);
}

uint64_t RaopClient::getNtpTimestamp()
{
    // NTP timestamp: seconds since Jan 1, 1900
    // Unix timestamp: seconds since Jan 1, 1970
    // Offset between them: 2208988800 seconds
    const uint64_t NTP_OFFSET = 2208988800ULL;
    
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count() % 1000000000;
    
    uint64_t ntpSeconds = seconds + NTP_OFFSET;
    uint64_t ntpFraction = (nanos << 32) / 1000000000;
    
    return (ntpSeconds << 32) | ntpFraction;
}

bool RaopClient::sendRtspRequest(const juce::String& method, const juce::String& uri, 
                                  const juce::StringPairArray& headers)
{
    juce::String request = method + " " + uri + " RTSP/1.0\r\n";
    
    for (int i = 0; i < headers.size(); ++i)
        request += headers.getAllKeys()[i] + ": " + headers.getAllValues()[i] + "\r\n";
    
    request += "\r\n";
    
    int sent = rtspSocket->write(request.toRawUTF8(), request.length());
    return sent == request.length();
}

bool RaopClient::receiveRtspResponse(juce::StringPairArray& headers, juce::String& body)
{
    char buffer[4096];
    juce::String response;
    
    // Read response with timeout
    int totalRead = 0;
    int timeout = 5000; // 5 seconds
    uint32_t startTime = juce::Time::getMillisecondCounter();
    
    while (juce::Time::getMillisecondCounter() - startTime < timeout)
    {
        int ready = rtspSocket->waitUntilReady(true, 100);
        if (ready < 0)
            return false;
        
        if (ready > 0)
        {
            int bytesRead = rtspSocket->read(buffer + totalRead, sizeof(buffer) - totalRead - 1, false);
            if (bytesRead <= 0)
                break;
            
            totalRead += bytesRead;
            buffer[totalRead] = '\0';
            
            // Check if we have complete response (ends with \r\n\r\n)
            if (strstr(buffer, "\r\n\r\n") != nullptr)
                break;
        }
    }
    
    if (totalRead == 0)
        return false;
    
    response = juce::String::fromUTF8(buffer, totalRead);
    
    // Parse status line
    int firstNewline = response.indexOf("\r\n");
    if (firstNewline < 0)
        return false;
    
    juce::String statusLine = response.substring(0, firstNewline);
    
    // Check for "RTSP/1.0 200 OK"
    if (!statusLine.contains("200"))
    {
        lastError = "RTSP error: " + statusLine;
        return false;
    }
    
    // Parse headers
    int headerEnd = response.indexOf("\r\n\r\n");
    juce::String headerSection = response.substring(firstNewline + 2, headerEnd);
    
    juce::StringArray lines = juce::StringArray::fromLines(headerSection);
    for (const auto& line : lines)
    {
        int colonPos = line.indexOf(":");
        if (colonPos > 0)
        {
            juce::String key = line.substring(0, colonPos).trim();
            juce::String value = line.substring(colonPos + 1).trim();
            headers.set(key, value);
        }
    }
    
    // Parse body if present
    if (headerEnd >= 0 && headerEnd + 4 < response.length())
    {
        body = response.substring(headerEnd + 4);
    }
    
    return true;
}

bool RaopClient::sendSetup()
{
    juce::StringPairArray headers;
    headers.set("CSeq", juce::String(cseq++));
    
    // Request UDP transport with our local ports
    juce::String transport = juce::String::formatted(
        "RTP/AVP/UDP;unicast;interleaved=0-1;mode=record;control_port=%d;timing_port=%d",
        localControlPort, localTimingPort
    );
    headers.set("Transport", transport);
    
    if (!sendRtspRequest("SETUP", "rtsp://" + currentDevice.getHostAddress() + "/stream", headers))
        return false;
    
    // Parse response to get server ports
    juce::StringPairArray responseHeaders;
    juce::String body;
    
    if (!receiveRtspResponse(responseHeaders, body))
        return false;
    
    // Extract session ID
    sessionId = responseHeaders["Session"];
    if (sessionId.isEmpty())
    {
        lastError = "No session ID in SETUP response";
        return false;
    }
    
    // Parse Transport header to get server ports
    juce::String transportResponse = responseHeaders["Transport"];
    if (transportResponse.isNotEmpty())
    {
        // Example: Transport: RTP/AVP/UDP;unicast;mode=record;server_port=6000;control_port=6001;timing_port=6002
        
        auto extractPort = [](const juce::String& str, const juce::String& key) -> int {
            int pos = str.indexOf(key + "=");
            if (pos >= 0)
            {
                int start = pos + key.length() + 1;
                int end = str.indexOfChar(start, ';');
                if (end < 0) end = str.length();
                return str.substring(start, end).getIntValue();
            }
            return 0;
        };
        
        serverAudioPort = extractPort(transportResponse, "server_port");
        serverControlPort = extractPort(transportResponse, "control_port");
        serverTimingPort = extractPort(transportResponse, "timing_port");
        
        // Some implementations use different naming
        if (serverAudioPort == 0)
            serverAudioPort = extractPort(transportResponse, "port");
    }
    
    return true;
}

bool RaopClient::sendRecord()
{
    juce::StringPairArray headers;
    headers.set("CSeq", juce::String(cseq++));
    headers.set("Session", sessionId);
    headers.set("Range", "npt=0-");
    
    // Set initial RTP info
    juce::String rtpInfo = juce::String::formatted("seq=%d;rtptime=%d", 
        sequenceNumber, static_cast<uint32_t>(samplesTransmitted));
    headers.set("RTP-Info", rtpInfo);
    
    if (!sendRtspRequest("RECORD", "rtsp://" + currentDevice.getHostAddress() + "/stream", headers))
        return false;
    
    // Parse response
    juce::StringPairArray responseHeaders;
    juce::String body;
    
    return receiveRtspResponse(responseHeaders, body);
}

bool RaopClient::sendTeardown()
{
    juce::StringPairArray headers;
    headers.set("CSeq", juce::String(cseq++));
    headers.set("Session", sessionId);
    
    bool result = sendRtspRequest("TEARDOWN", "rtsp://" + currentDevice.getHostAddress() + "/stream", headers);
    
    // Don't wait for response during teardown, just close
    return result;
}
