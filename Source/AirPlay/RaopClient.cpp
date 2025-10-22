#include "RaopClient.h"

RaopClient::RaopClient()
{
    socket = std::make_unique<juce::StreamingSocket>();
    audioSocket = std::make_unique<juce::DatagramSocket>();
    controlSocket = std::make_unique<juce::DatagramSocket>();
    timingSocket = std::make_unique<juce::DatagramSocket>();
    auth = std::make_unique<AirPlayAuth>();

    // Initialize random SSRC
    juce::Random random;
    ssrc = random.nextInt();
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
    cseq = 1;  // Reset sequence number

    // Initialize authentication if enabled
    if (useAuthentication)
    {
        if (!auth->initialize())
        {
            lastError = "Failed to initialize authentication: " + auth->getLastError();
            return false;
        }

        // Set device password if required
        if (device.requiresPassword())
        {
            auth->setPassword(device.getPassword());
        }
    }

    // Create UDP sockets first
    if (!createUdpSockets())
    {
        lastError = "Failed to create UDP sockets";
        return false;
    }

    if (!socket->connect(device.getHostAddress(), device.getPort(), 5000))
    {
        lastError = "Failed to connect to " + device.getHostAddress();
        closeUdpSockets();
        return false;
    }

    // Initialize timing
    startTime = juce::Time::getCurrentTime();
    sequenceNumber = 0;
    rtpTimestamp = 0;

    // Perform RTSP handshake with authentication
    if (useAuthentication)
    {
        if (!sendOptions())
        {
            disconnect();
            return false;
        }

        if (!sendAnnounce())
        {
            disconnect();
            return false;
        }
    }

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
    return true;
}

void RaopClient::disconnect()
{
    if (connected)
    {
        sendTeardown();
        socket->close();
        closeUdpSockets();
        connected = false;
    }
}

bool RaopClient::isConnected() const
{
    return connected;
}

bool RaopClient::sendAudio(const juce::MemoryBlock& audioData, int /*sampleRate*/, int channels)
{
    if (!connected || !audioSocket || serverPort == 0)
        return false;

    // Calculate RTP timestamp increment based on sample rate
    // RTP timestamps increment at the sample rate, so increment by samples per packet
    const int samplesPerPacket = audioData.getSize() / (channels * 2); // 16-bit samples
    const uint32_t timestampIncrement = samplesPerPacket;

    // Build RTP header
    RTPHeader header;
    header.version_flags = 0x80;  // Version 2, no padding, no extension, no CSRC
    // Set marker bit for first packet or based on stream requirements
    bool markerBit = (sequenceNumber == 0);
    header.payload_type = 0x60 | (markerBit ? 0x80 : 0x00);  // Apple Lossless payload type (96) with marker bit
    header.sequence_number = juce::ByteOrder::swapIfBigEndian(sequenceNumber);
    header.timestamp = juce::ByteOrder::swapIfBigEndian(rtpTimestamp);
    header.ssrc = juce::ByteOrder::swapIfBigEndian(ssrc);

    // Create packet buffer
    const size_t headerSize = sizeof(RTPHeader);
    const size_t packetSize = headerSize + audioData.getSize();
    juce::MemoryBlock packet(packetSize);

    // Copy header
    memcpy(packet.getData(), &header, headerSize);

    // Copy audio data
    memcpy(static_cast<uint8_t*>(packet.getData()) + headerSize,
           audioData.getData(), audioData.getSize());

    // Send the RTP packet
    if (!sendRtpPacket(packet.getData(), packetSize))
    {
        lastError = "Failed to send RTP packet";
        return false;
    }

    // Update sequence number and timestamp for next packet
    sequenceNumber++;
    rtpTimestamp += timestampIncrement;

    return true;
}

bool RaopClient::sendRtspRequest(const juce::String& method, const juce::String& uri,
                                  const juce::StringPairArray& headers, RtspResponse* response)
{
    return sendRtspRequest(method, uri, headers, juce::String(), response);
}

bool RaopClient::sendRtspRequest(const juce::String& method, const juce::String& uri,
                                  const juce::StringPairArray& headers, const juce::String& body,
                                  RtspResponse* response)
{
    juce::String request = method + " " + uri + " RTSP/1.0\r\n";

    for (int i = 0; i < headers.size(); ++i)
        request += headers.getAllKeys()[i] + ": " + headers.getAllValues()[i] + "\r\n";

    // Add body if present
    if (body.isNotEmpty())
    {
        request += "Content-Length: " + juce::String(body.length()) + "\r\n";
    }

    request += "\r\n";

    if (body.isNotEmpty())
        request += body;

    int sent = socket->write(request.toRawUTF8(), request.length());
    if (sent != request.length())
    {
        lastError = "Failed to send RTSP request";
        return false;
    }

    // Read response if requested
    if (response != nullptr)
    {
        char buffer[4096];
        int bytesRead = socket->read(buffer, sizeof(buffer) - 1, false);
        if (bytesRead <= 0)
        {
            lastError = "Failed to read RTSP response";
            return false;
        }

        buffer[bytesRead] = '\0';
        juce::String responseText(buffer);

        if (!parseRtspResponse(responseText, *response))
        {
            lastError = "Failed to parse RTSP response";
            return false;
        }

        if (!response->isSuccess())
        {
            lastError = "RTSP request failed: " + juce::String(response->statusCode) + " " + response->statusMessage;
            return false;
        }
    }

    return true;
}

bool RaopClient::parseRtspResponse(const juce::String& responseText, RtspResponse& response)
{
    juce::StringArray lines = juce::StringArray::fromLines(responseText);
    if (lines.isEmpty())
        return false;

    // Parse status line: RTSP/1.0 200 OK
    juce::String statusLine = lines[0].trim();
    juce::StringArray statusParts;
    statusParts.addTokens(statusLine, " ", "");
    
    if (statusParts.size() < 2)
        return false;

    response.statusCode = statusParts[1].getIntValue();
    if (statusParts.size() >= 3)
        response.statusMessage = statusLine.fromFirstOccurrenceOf(statusParts[1], false, false).trim();

    // Parse headers
    int i = 1;
    for (; i < lines.size(); ++i)
    {
        juce::String line = lines[i].trim();
        if (line.isEmpty())
        {
            i++; // Move past the empty line
            break;
        }

        int colonIndex = line.indexOf(":");
        if (colonIndex > 0)
        {
            juce::String key = line.substring(0, colonIndex).trim();
            juce::String value = line.substring(colonIndex + 1).trim();
            response.headers.set(key, value);
        }
    }

    // Parse body (if any)
    if (i < lines.size())
    {
        juce::String bodyLines;
        for (; i < lines.size(); ++i)
            bodyLines += lines[i] + "\n";
        response.body = bodyLines.trim();
    }

    return true;
}

bool RaopClient::parseTransportHeader(const juce::String& transport, int& audioPort, int& controlPort, int& timingPort)
{
    // Example: RTP/AVP/UDP;unicast;server_port=6000-6001;control_port=6001;timing_port=6002
    // or: RTP/AVP/UDP;unicast;server_port=6000-6001
    
    audioPort = 0;
    controlPort = 0;
    timingPort = 0;

    // Parse server_port parameter
    int serverPortIndex = transport.indexOf("server_port=");
    if (serverPortIndex >= 0)
    {
        juce::String serverPortsStr = transport.substring(serverPortIndex + 12);
        int dashIndex = serverPortsStr.indexOf("-");
        
        if (dashIndex > 0)
        {
            // Extract audio port
            audioPort = serverPortsStr.substring(0, dashIndex).getIntValue();
            
            // Extract control port
            juce::String controlPortStr = serverPortsStr.substring(dashIndex + 1);
            int semicolonIndex = controlPortStr.indexOf(";");
            int spaceIndex = controlPortStr.indexOfChar(' ');
            int endIndex = -1;
            
            if (semicolonIndex >= 0 && spaceIndex >= 0)
                endIndex = juce::jmin(semicolonIndex, spaceIndex);
            else if (semicolonIndex >= 0)
                endIndex = semicolonIndex;
            else if (spaceIndex >= 0)
                endIndex = spaceIndex;
            
            if (endIndex > 0)
                controlPortStr = controlPortStr.substring(0, endIndex);
            
            controlPort = controlPortStr.trim().getIntValue();
        }
    }

    // Parse timing_port parameter (if present)
    int timingPortIndex = transport.indexOf("timing_port=");
    if (timingPortIndex >= 0)
    {
        juce::String timingPortStr = transport.substring(timingPortIndex + 12);
        int semicolonIndex = timingPortStr.indexOf(";");
        int spaceIndex = timingPortStr.indexOfChar(' ');
        
        if (semicolonIndex >= 0)
            timingPortStr = timingPortStr.substring(0, semicolonIndex);
        else if (spaceIndex >= 0)
            timingPortStr = timingPortStr.substring(0, spaceIndex);
        
        timingPort = timingPortStr.trim().getIntValue();
    }
    // Some servers might specify timing port as the third port in server_port range
    else if (audioPort > 0 && controlPort > 0)
    {
        // Default timing port to control port + 1 if not explicitly specified.
        // This follows the common RAOP/AirPlay convention where ports are allocated
        // sequentially: audio (N), control (N+1), timing (N+2).
        // Reference: Apple's RAOP protocol implementation
        timingPort = controlPort + 1;
    }

    return audioPort > 0 && controlPort > 0;
}

bool RaopClient::sendOptions()
{
    juce::StringPairArray headers;
    headers.set("CSeq", juce::String(cseq++));
    headers.set("User-Agent", "FreeCaster/1.0");

    // Add Apple-Challenge for authentication
    if (useAuthentication && auth->isInitialized())
    {
        juce::String challenge = auth->generateChallenge();
        if (challenge.isNotEmpty())
            headers.set("Apple-Challenge", challenge);
    }

    RtspResponse response;
    if (!sendRtspRequest("OPTIONS", "*", headers, &response))
        return false;

    // Verify Apple-Response if authentication is enabled
    if (useAuthentication && response.headers.containsKey("Apple-Response"))
    {
        juce::String appleResponse = response.headers["Apple-Response"];
        if (!auth->verifyResponse(appleResponse, "", currentDevice.getHostAddress()))
        {
            lastError = "Authentication failed: Invalid Apple-Response";
            return false;
        }
    }

    return true;
}

bool RaopClient::sendAnnounce()
{
    juce::StringPairArray headers;
    headers.set("CSeq", juce::String(cseq++));
    headers.set("Content-Type", "application/sdp");
    headers.set("User-Agent", "FreeCaster/1.0");

    // Build SDP (Session Description Protocol) body
    juce::String sdp;
    sdp += "v=0\r\n";
    sdp += "o=FreeCaster 0 0 IN IP4 127.0.0.1\r\n";
    sdp += "s=FreeCaster Audio Stream\r\n";
    sdp += "c=IN IP4 " + currentDevice.getHostAddress() + "\r\n";
    sdp += "t=0 0\r\n";
    
    // Media description
    sdp += "m=audio 0 RTP/AVP 96\r\n";
    sdp += "a=rtpmap:96 AppleLossless\r\n";
    sdp += "a=fmtp:96 352 0 16 40 10 14 2 255 0 0 44100\r\n";

    // Add RSA public key for encryption (AES-RSA)
    if (useAuthentication && auth->isInitialized())
    {
        juce::String publicKey = auth->getPublicKeyBase64();
        if (publicKey.isNotEmpty())
        {
            sdp += "a=rsaaeskey:" + publicKey + "\r\n";
        }

        // Add AES IV (initialization vector)
        // For now, use a simple IV - in production this should be random
        juce::String aesIV = "AAAAAAAAAAAAAAAAAAAAAA==";  // Base64 encoded zeros
        sdp += "a=aesiv:" + aesIV + "\r\n";
    }

    RtspResponse response;
    return sendRtspRequest("ANNOUNCE", "rtsp://" + currentDevice.getHostAddress() + "/stream",
                          headers, sdp, &response);
}

bool RaopClient::sendSetup()
{
    juce::StringPairArray headers;
    headers.set("CSeq", juce::String(cseq++));

    // Specify client UDP ports for server to send to
    juce::String transport = "RTP/AVP/UDP;unicast;interleaved=0-1;mode=record;";
    transport += "client_port=" + juce::String(clientAudioPort);
    transport += "-" + juce::String(clientControlPort);

    headers.set("Transport", transport);

    RtspResponse response;
    if (!sendRtspRequest("SETUP", "rtsp://" + currentDevice.getHostAddress() + "/stream", headers, &response))
        return false;

    // Parse Transport header for server ports
    juce::String transportResponse = response.headers["Transport"];
    if (transportResponse.isEmpty())
    {
        lastError = "Server did not provide Transport header in SETUP response";
        return false;
    }

    if (!parseTransportHeader(transportResponse, serverPort, controlPort, timingPort))
    {
        lastError = "Failed to parse server ports from Transport header";
        return false;
    }

    // Extract session ID from Session header
    juce::String sessionHeader = response.headers["Session"];
    if (sessionHeader.isNotEmpty())
    {
        int semicolonIndex = sessionHeader.indexOf(";");
        if (semicolonIndex > 0)
            session = sessionHeader.substring(0, semicolonIndex).trim();
        else
            session = sessionHeader.trim();
    }
    else
    {
        lastError = "Server did not provide Session ID in SETUP response";
        return false;
    }

    return true;
}

bool RaopClient::sendRecord()
{
    juce::StringPairArray headers;
    headers.set("CSeq", juce::String(cseq++));
    headers.set("Range", "npt=0-");
    headers.set("RTP-Info", "seq=0;rtptime=0");
    
    if (session.isNotEmpty())
        headers.set("Session", session);

    return sendRtspRequest("RECORD", "rtsp://" + currentDevice.getHostAddress() + "/stream", headers);
}

bool RaopClient::sendTeardown()
{
    juce::StringPairArray headers;
    headers.set("CSeq", juce::String(cseq++));
    
    if (session.isNotEmpty())
        headers.set("Session", session);

    return sendRtspRequest("TEARDOWN", "rtsp://" + currentDevice.getHostAddress() + "/stream", headers);
}

bool RaopClient::createUdpSockets()
{
    // Create audio socket
    if (!audioSocket->bindToPort(clientAudioPort))
    {
        lastError = "Failed to bind audio socket to port " + juce::String(clientAudioPort);
        return false;
    }

    // Create control socket
    if (!controlSocket->bindToPort(clientControlPort))
    {
        lastError = "Failed to bind control socket to port " + juce::String(clientControlPort);
        return false;
    }

    // Create timing socket
    if (!timingSocket->bindToPort(clientTimingPort))
    {
        lastError = "Failed to bind timing socket to port " + juce::String(clientTimingPort);
        return false;
    }

    return true;
}

void RaopClient::closeUdpSockets()
{
    if (audioSocket) audioSocket->shutdown();
    if (controlSocket) controlSocket->shutdown();
    if (timingSocket) timingSocket->shutdown();
}

bool RaopClient::sendRtpPacket(const void* data, size_t size)
{
    if (!audioSocket || serverPort == 0)
        return false;

    return audioSocket->write(currentDevice.getHostAddress(), serverPort, data, (int)size) == (int)size;
}

RaopClient::NTPTimestamp RaopClient::getCurrentNtpTimestamp() const
{
    // Convert current time to NTP timestamp (seconds since 1900)
    // NTP epoch is 1900, Unix epoch is 1970, so add 70 years worth of seconds
    juce::Time currentTime = juce::Time::getCurrentTime();
    juce::Time ntpEpoch(1900, 0, 1, 0, 0, 0, 0, false);

    double secondsSinceNtpEpoch = (currentTime.toMilliseconds() - ntpEpoch.toMilliseconds()) / 1000.0;

    RaopClient::NTPTimestamp ntp;
    ntp.seconds = (uint32_t)secondsSinceNtpEpoch;
    ntp.fraction = (uint32_t)((secondsSinceNtpEpoch - ntp.seconds) * 0x100000000ULL);
    return ntp;
}

void RaopClient::setPassword(const juce::String& password)
{
    if (auth)
        auth->setPassword(password);
}
