#include "RaopClient.h"

RaopClient::RaopClient()
{
    socket = std::make_unique<juce::StreamingSocket>();
    audioSocket = std::make_unique<juce::DatagramSocket>();
    controlSocket = std::make_unique<juce::DatagramSocket>();
    timingSocket = std::make_unique<juce::DatagramSocket>();

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

bool RaopClient::sendAudio(const juce::MemoryBlock& audioData, int sampleRate, int channels)
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
                                  const juce::StringPairArray& headers, juce::StringPairArray* responseHeaders)
{
    juce::String request = method + " " + uri + " RTSP/1.0\r\n";

    for (int i = 0; i < headers.size(); ++i)
        request += headers.getAllKeys()[i] + ": " + headers.getAllValues()[i] + "\r\n";

    request += "\r\n";

    int sent = socket->write(request.toRawUTF8(), request.length());
    if (sent != request.length())
        return false;

    // Read response if headers are requested
    if (responseHeaders != nullptr)
    {
        char buffer[4096];
        int bytesRead = socket->read(buffer, sizeof(buffer) - 1, false);
        if (bytesRead <= 0)
            return false;

        buffer[bytesRead] = '\0';
        juce::String response(buffer);

        // Parse response headers
        juce::StringArray lines = juce::StringArray::fromLines(response);
        for (int i = 1; i < lines.size(); ++i) // Skip status line
        {
            juce::String line = lines[i].trim();
            if (line.isEmpty())
                break;

            int colonIndex = line.indexOf(":");
            if (colonIndex > 0)
            {
                juce::String key = line.substring(0, colonIndex).trim();
                juce::String value = line.substring(colonIndex + 1).trim();
                responseHeaders->set(key, value);
            }
        }
    }

    return true;
}

bool RaopClient::sendSetup()
{
    juce::StringPairArray headers;
    headers.set("CSeq", "1");

    // Specify client UDP ports for server to send to
    juce::String transport = "RTP/AVP/UDP;unicast;interleaved=0-1;mode=record;";
    transport += "client_port=" + juce::String(clientAudioPort);
    transport += "-" + juce::String(clientControlPort);

    headers.set("Transport", transport);

    juce::StringPairArray responseHeaders;
    if (!sendRtspRequest("SETUP", "rtsp://" + currentDevice.getHostAddress() + "/stream", headers, &responseHeaders))
        return false;

    // Parse server ports from Transport header
    juce::String transportResponse = responseHeaders["Transport"];
    if (transportResponse.isNotEmpty())
    {
        // Look for server_port=xxxx-yyyy
        int serverPortIndex = transportResponse.indexOf("server_port=");
        if (serverPortIndex >= 0)
        {
            juce::String serverPortsStr = transportResponse.substring(serverPortIndex + 12);
            int dashIndex = serverPortsStr.indexOf("-");
            if (dashIndex > 0)
            {
                serverPort = serverPortsStr.substring(0, dashIndex).getIntValue();
                
                // Extract control port with proper whitespace and parameter handling
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
                controlPortStr = controlPortStr.trim();
                controlPort = controlPortStr.getIntValue();
            }
        }

        // Extract session ID from Session header
        juce::String sessionHeader = responseHeaders["Session"];
        if (sessionHeader.isNotEmpty())
        {
            int semicolonIndex = sessionHeader.indexOf(";");
            if (semicolonIndex > 0)
                session = sessionHeader.substring(0, semicolonIndex);
            else
                session = sessionHeader;
        }
    }

    return true;
}

bool RaopClient::sendRecord()
{
    juce::StringPairArray headers;
    headers.set("CSeq", "2");
    headers.set("Range", "npt=0-");
    headers.set("RTP-Info", "seq=0;rtptime=0");

    return sendRtspRequest("RECORD", "rtsp://" + currentDevice.getHostAddress() + "/stream", headers);
}

bool RaopClient::sendTeardown()
{
    juce::StringPairArray headers;
    headers.set("CSeq", "3");

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

NTPTimestamp RaopClient::getCurrentNtpTimestamp() const
{
    // Convert current time to NTP timestamp (seconds since 1900)
    // NTP epoch is 1900, Unix epoch is 1970, so add 70 years worth of seconds
    juce::Time currentTime = juce::Time::getCurrentTime();
    juce::Time ntpEpoch(1900, 0, 1, 0, 0, 0, 0, false);

    double secondsSinceNtpEpoch = (currentTime.toMilliseconds() - ntpEpoch.toMilliseconds()) / 1000.0;
    
    NTPTimestamp ntp;
    ntp.seconds = (uint32_t)secondsSinceNtpEpoch;
    ntp.fraction = (uint32_t)((secondsSinceNtpEpoch - ntp.seconds) * 0x100000000ULL);
    return ntp;
}
