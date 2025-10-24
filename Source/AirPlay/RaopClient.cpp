#include "RaopClient.h"
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/rsa.h>

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

    // Initialize client identity values expected by some RAOP servers
    // Generate pseudo-stable IDs for the process lifetime
    juce::String hex = juce::String::toHexString(juce::Random::getSystemRandom().nextInt64());
    hex = hex.toUpperCase();
    if (hex.length() < 16) hex = hex.paddedLeft('0', 16);
    clientInstanceId = hex.substring(0, 16);
    dacpId = clientInstanceId;

    // Apple-Device-ID as MAC-like format from random
    juce::String macHex = juce::String::toHexString(juce::Random::getSystemRandom().nextInt64()).toUpperCase();
    if (macHex.length() < 12) macHex = macHex.paddedLeft('0', 12);
    macHex = macHex.substring(0, 12);
    appleDeviceId = macHex.substring(0,2)+":"+macHex.substring(2,4)+":"+macHex.substring(4,6)+":"+
                    macHex.substring(6,8)+":"+macHex.substring(8,10)+":"+macHex.substring(10,12);
}

RaopClient::~RaopClient()
{
    disconnect();
}

bool RaopClient::connect(const AirPlayDevice& device)
{
    juce::Logger::writeToLog("[RaopClient] connect() called for device: " + device.getDeviceName() + " at " + device.getHostAddress() + ":" + juce::String(device.getPort()));

    if (connected)
        disconnect();

    setConnectionState(ConnectionState::Connecting);
    lastConnectionAttemptTime = juce::Time::currentTimeMillis();
    consecutiveFailures = 0;
    reconnectAttempts = 0;

    currentDevice = device;
    cseq = 1;  // Reset sequence number
    receivedAppleResponse = false;  // Reset auth response flag
    juce::Logger::writeToLog("[RaopClient] Connection state set to Connecting");

    // Initialize authentication if enabled
    if (useAuthentication)
    {
        if (!auth->initialize())
        {
            lastError = "Failed to initialize authentication: " + auth->getLastError();
            juce::Logger::writeToLog("RaopClient: Auth initialization failed, disabling auth");
            useAuthentication = false;  // Disable auth if initialization fails
        }
        else
        {
            // Set device password if required
            if (device.requiresPassword())
            {
                auth->setPassword(device.getPassword());
            }
        }
    }

    // Create UDP sockets first
    if (!createUdpSockets())
    {
        lastError = "Failed to create UDP sockets";
        return false;
    }

    // Try to connect with timeout
    juce::Logger::writeToLog("RaopClient: Attempting TCP connection to " + device.getHostAddress() + ":" + juce::String(device.getPort()));
    if (!socket->connect(device.getHostAddress(), device.getPort(), 10000))
    {
        lastError = "Failed to connect to " + device.getHostAddress();
        logError(lastError);
        juce::Logger::writeToLog("RaopClient: TCP connection failed - device may be unreachable or not accepting connections");
        setConnectionState(ConnectionState::TimedOut);
        closeUdpSockets();
        return false;
    }
    juce::Logger::writeToLog("RaopClient: TCP connection established successfully");

    // Skip socket readiness check - AirPlay devices expect immediate RTSP handshake
    juce::Logger::writeToLog("RaopClient: TCP connection established, proceeding with RTSP handshake");

    // Initialize timing
    startTime = juce::Time::getCurrentTime();
    sequenceNumber = 0;
    rtpTimestamp = 0;

    // Perform RTSP handshake with authentication
    juce::Logger::writeToLog("RaopClient: Starting RTSP handshake");

    // Always send OPTIONS request (part of RTSP standard)
    juce::Logger::writeToLog("RaopClient: Sending OPTIONS request");
    if (!sendOptions())
    {
        juce::Logger::writeToLog("RaopClient: OPTIONS request failed");
        disconnect();
        return false;
    }

    // Send ANNOUNCE request with authentication if enabled
    juce::Logger::writeToLog("RaopClient: Sending ANNOUNCE request");
    if (!sendAnnounce())
    {
        juce::Logger::writeToLog("RaopClient: ANNOUNCE request failed");
        disconnect();
        return false;
    }

    juce::Logger::writeToLog("RaopClient: Sending SETUP request");
    if (!sendSetup())
    {
        juce::Logger::writeToLog("RaopClient: SETUP request failed");
        disconnect();
        return false;
    }

    juce::Logger::writeToLog("RaopClient: Sending RECORD request");
    if (!sendRecord())
    {
        disconnect();
        return false;
    }

    connected = true;
    setConnectionState(ConnectionState::Connected);
    lastSuccessfulSendTime = juce::Time::currentTimeMillis();
    consecutiveFailures = 0;
    reconnectAttempts = 0;
    juce::Logger::writeToLog("RaopClient: Successfully connected to " + device.getDeviceName());
    return true;
}

void RaopClient::disconnect()
{
    const juce::ScopedLock sl(stateLock);

    if (connected)
    {
        sendTeardown();
        socket->close();
        closeUdpSockets();
        connected = false;
        setConnectionState(ConnectionState::Disconnected);
        juce::Logger::writeToLog("RaopClient: Disconnected from " + currentDevice.getDeviceName());
    }
}

bool RaopClient::isConnected() const
{
    return connected;
}

bool RaopClient::sendAudio(const juce::MemoryBlock& audioData, int /*sampleRate*/, int channels)
{
    if (!connected || !audioSocket || serverPort == 0)
    {
        consecutiveFailures++;
        if (consecutiveFailures > maxConsecutiveFailures && autoReconnectEnabled)
        {
            logError("Too many consecutive send failures, attempting reconnect");
            attemptReconnect();
        }
        return false;
    }

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
        consecutiveFailures++;

        if (consecutiveFailures > maxConsecutiveFailures)
        {
            logError("Network send failure detected, marking connection as error");
            setConnectionState(ConnectionState::Error);

            if (autoReconnectEnabled)
            {
                attemptReconnect();
            }
        }
        return false;
    }

    // Update sequence number and timestamp for next packet
    sequenceNumber++;
    rtpTimestamp += timestampIncrement;
    lastSuccessfulSendTime = juce::Time::currentTimeMillis();
    consecutiveFailures = 0;  // Reset on successful send

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

    // Inject common RAOP headers some servers expect
    juce::StringPairArray enriched = headers;
    if (!clientInstanceId.isEmpty() && !enriched.containsKey("Client-Instance"))
        enriched.set("Client-Instance", clientInstanceId);
    if (!dacpId.isEmpty() && !enriched.containsKey("DACP-ID"))
        enriched.set("DACP-ID", dacpId);
    if (!appleDeviceId.isEmpty() && !enriched.containsKey("Apple-Device-ID"))
        enriched.set("Apple-Device-ID", appleDeviceId);

    for (int i = 0; i < enriched.size(); ++i)
        request += enriched.getAllKeys()[i] + ": " + enriched.getAllValues()[i] + "\r\n";

    // Add body if present
    if (body.isNotEmpty())
    {
        request += "Content-Length: " + juce::String(body.length()) + "\r\n";
    }

    request += "\r\n";

    if (body.isNotEmpty())
        request += body;

    juce::Logger::writeToLog("RaopClient: Sending RTSP request: " + request);
    int sent = socket->write(request.toRawUTF8(), request.length());
    if (sent != request.length())
    {
        lastError = "Failed to send RTSP request";
        juce::Logger::writeToLog("RaopClient: Failed to send RTSP request - sent " + juce::String(sent) + " of " + juce::String(request.length()) + " bytes");
        return false;
    }
    juce::Logger::writeToLog("RaopClient: Successfully sent " + juce::String(sent) + " bytes");

    // Read response if requested
    if (response != nullptr)
    {
        // Wait for the socket to be ready for reading (timeout: 5 seconds)
        juce::Logger::writeToLog("RaopClient: Waiting for RTSP response...");
        int ready = socket->waitUntilReady(true, 5000);
        if (ready != 1)
        {
            lastError = "Socket not ready for reading (timeout or error)";
            juce::Logger::writeToLog("RaopClient: Socket not ready - ready state: " + juce::String(ready));
            return false;
        }

        char buffer[4096];
        int bytesRead = socket->read(buffer, sizeof(buffer) - 1, false);
        if (bytesRead <= 0)
        {
            lastError = "Failed to read RTSP response";
            juce::Logger::writeToLog("RaopClient: No response received from device after socket was ready");
            return false;
        }

        buffer[bytesRead] = '\0';
        juce::String responseText(buffer);
        juce::Logger::writeToLog("RaopClient: Received RTSP response: " + responseText);

        if (!parseRtspResponse(responseText, *response))
        {
            lastError = "Failed to parse RTSP response";
            juce::Logger::writeToLog("RaopClient: Failed to parse RTSP response: " + responseText);
            return false;
        }

        if (!response->isSuccess())
        {
            lastError = "RTSP request failed: " + juce::String(response->statusCode) + " " + response->statusMessage;
            juce::Logger::writeToLog("RaopClient: RTSP request failed with status " + juce::String(response->statusCode) + ": " + response->statusMessage);
            return false;
        }

        juce::Logger::writeToLog("RaopClient: RTSP request successful with status " + juce::String(response->statusCode));
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

    // Add Apple-Challenge for authentication (skip if auth failed to initialize)
    if (useAuthentication && auth && auth->isInitialized())
    {
        juce::String challenge = auth->generateChallenge();
        if (challenge.isNotEmpty())
            headers.set("Apple-Challenge", challenge);
    }

    RtspResponse response;
    if (!sendRtspRequest("OPTIONS", "*", headers, &response))
        return false;

    // Check if device sent Apple-Response
    bool deviceSupportsAuth = response.headers.containsKey("Apple-Response");
    receivedAppleResponse = deviceSupportsAuth;

    if (deviceSupportsAuth)
    {
        juce::String appleResponse = response.headers["Apple-Response"];
        juce::Logger::writeToLog("RaopClient: Received Apple-Response: " + appleResponse);
        if (useAuthentication && auth && auth->isInitialized())
        {
            if (!auth->verifyResponse(appleResponse, "", currentDevice.getHostAddress()))
            {
                lastError = "Authentication failed: Invalid Apple-Response";
                return false;
            }
        }
    }
    else
    {
        juce::Logger::writeToLog("RaopClient: Device did not send Apple-Response (auth not supported)");
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
    // Connection address should be client's local IP (127.0.0.1), not server's IP
    sdp += "c=IN IP4 127.0.0.1\r\n";
    sdp += "t=0 0\r\n";

    // Media description
    sdp += "m=audio 0 RTP/AVP 96\r\n";
    sdp += "a=rtpmap:96 AppleLossless\r\n";
    sdp += "a=fmtp:96 352 0 16 40 10 14 2 255 0 0 44100\r\n";

    // Generate proper RSA-OAEP encrypted AES session key for RAOP/AirPlay 1
    if (useAuthentication && auth && auth->isInitialized())
    {
        juce::String receiverPkBase64 = currentDevice.getServerPublicKey();
        juce::Logger::writeToLog("RaopClient: Server public key from TXT record: " + receiverPkBase64);
        
        if (receiverPkBase64.isNotEmpty())
        {
            // Parse the server's public key from TXT record (hex string)
            juce::MemoryBlock serverKeyData;
            serverKeyData.loadFromHexString(receiverPkBase64);
            
            // Create RSA key from the hex data
            const unsigned char* keyData = static_cast<const unsigned char*>(serverKeyData.getData());
            RSA* rsa = d2i_RSAPublicKey(nullptr, &keyData, (long)serverKeyData.getSize());
            
            if (rsa)
            {
                juce::Logger::writeToLog("RaopClient: Successfully parsed server public key from hex");
                
                // Generate random 16-byte AES session key and IV
                juce::MemoryBlock aesKey(16), aesIV(16);
                RAND_bytes(static_cast<unsigned char*>(aesKey.getData()), 16);
                RAND_bytes(static_cast<unsigned char*>(aesIV.getData()), 16);
                
                // Encrypt AES key with RSA-OAEP
                juce::MemoryBlock encrypted(128); // 1024-bit RSA = 128 bytes ciphertext
                int encryptedLen = RSA_public_encrypt(16, static_cast<unsigned char*>(aesKey.getData()),
                                                    static_cast<unsigned char*>(encrypted.getData()),
                                                    rsa, RSA_PKCS1_OAEP_PADDING);
                
                if (encryptedLen > 0)
                {
                    encrypted.setSize(encryptedLen);
                    juce::String rsaaeskey = juce::Base64::toBase64(encrypted.getData(), (size_t)encrypted.getSize());
                    juce::String aesiv = juce::Base64::toBase64(aesIV.getData(), (size_t)aesIV.getSize());
                    
                    sdp += "a=rsaaeskey:" + rsaaeskey + "\r\n";
                    sdp += "a=aesiv:" + aesiv + "\r\n";
                    juce::Logger::writeToLog("RaopClient: Added proper RSA-OAEP encrypted AES session key");
                }
                else
                {
                    juce::Logger::writeToLog("RaopClient: Failed to encrypt AES key with RSA-OAEP");
                }
                
                RSA_free(rsa);
            }
            else
            {
                juce::Logger::writeToLog("RaopClient: Failed to parse server public key from hex");
            }
        }
        else
        {
            // Fallback: send our public key if no server key available
            // Some devices (like native Sonos) need auth fields even without server key
            juce::String publicKey = auth->getPublicKeyBase64();
            if (publicKey.isNotEmpty())
            {
                sdp += "a=rsaaeskey:" + publicKey + "\r\n";
                juce::String aesIV = "AAAAAAAAAAAAAAAAAAAAAA==";  // Base64 encoded zeros
                sdp += "a=aesiv:" + aesIV + "\r\n";
                juce::Logger::writeToLog("RaopClient: Using fallback auth fields (no server key)");
            }
        }
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
    // Try different transport formats based on device type
    juce::String transport;
    if (currentDevice.getHostAddress().contains("airsonos"))
    {
        // Airsonos bridge format
        transport = "RTP/AVP/UDP;unicast;mode=record;";
        transport += "client_port=" + juce::String(clientAudioPort);
        transport += "-" + juce::String(clientControlPort);
        transport += ";interleaved=0-1";
    }
    else
    {
        // Native AirPlay device format
        transport = "RTP/AVP/UDP;unicast;mode=record;";
        transport += "client_port=" + juce::String(clientAudioPort);
        transport += "-" + juce::String(clientControlPort);
    }

    headers.set("Transport", transport);

    RtspResponse response;
    if (!sendRtspRequest("SETUP", "rtsp://" + currentDevice.getHostAddress() + "/stream", headers, &response))
        return false;

    // Log response details for debugging
    juce::Logger::writeToLog("RaopClient: SETUP response status: " + juce::String(response.statusCode) + " " + response.statusMessage);
    juce::Logger::writeToLog("RaopClient: SETUP response headers:");
    for (int i = 0; i < response.headers.size(); ++i)
    {
        juce::Logger::writeToLog("  " + response.headers.getAllKeys()[i] + ": " + response.headers.getAllValues()[i]);
    }

    // Parse Transport header for server ports
    juce::String transportResponse = response.headers["Transport"];
    if (transportResponse.isEmpty())
    {
        lastError = "Server did not provide Transport header in SETUP response";
        juce::Logger::writeToLog("RaopClient: SETUP response body: " + response.body);
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
    // Close any existing sockets first
    closeUdpSockets();

    // Try to select available ports first to avoid bind failures
    selectAvailableClientPorts();
    juce::Logger::writeToLog("RaopClient: Creating UDP sockets - audio:" + juce::String(clientAudioPort) +
        " control:" + juce::String(clientControlPort) + " timing:" + juce::String(clientTimingPort));

    // Validate socket objects exist
    if (!audioSocket)
    {
        lastError = "Audio socket is null";
        juce::Logger::writeToLog("RaopClient ERROR: " + lastError);
        return false;
    }
    if (!controlSocket)
    {
        lastError = "Control socket is null";
        juce::Logger::writeToLog("RaopClient ERROR: " + lastError);
        return false;
    }
    if (!timingSocket)
    {
        lastError = "Timing socket is null";
        juce::Logger::writeToLog("RaopClient ERROR: " + lastError);
        return false;
    }

    // Create audio socket
    juce::Logger::writeToLog("RaopClient: Binding audio socket to port " + juce::String(clientAudioPort) +
        " (socket at " + juce::String::toHexString(reinterpret_cast<uintptr_t>(audioSocket.get())) + ")");
    if (!audioSocket->bindToPort(clientAudioPort))
    {
        lastError = "Failed to bind audio socket to port " + juce::String(clientAudioPort);
        juce::Logger::writeToLog("RaopClient ERROR: " + lastError);
        return false;
    }
    juce::Logger::writeToLog("RaopClient: Successfully bound audio socket to port " + juce::String(clientAudioPort));

    // Create control socket
    juce::Logger::writeToLog("RaopClient: Binding control socket to port " + juce::String(clientControlPort) +
        " (socket at " + juce::String::toHexString(reinterpret_cast<uintptr_t>(controlSocket.get())) + ")");
    if (!controlSocket->bindToPort(clientControlPort))
    {
        lastError = "Failed to bind control socket to port " + juce::String(clientControlPort);
        juce::Logger::writeToLog("RaopClient ERROR: " + lastError);
        return false;
    }
    juce::Logger::writeToLog("RaopClient: Successfully bound control socket to port " + juce::String(clientControlPort));

    // Create timing socket
    juce::Logger::writeToLog("RaopClient: Binding timing socket to port " + juce::String(clientTimingPort) +
        " (socket at " + juce::String::toHexString(reinterpret_cast<uintptr_t>(timingSocket.get())) + ")");
    if (!timingSocket->bindToPort(clientTimingPort))
    {
        lastError = "Failed to bind timing socket to port " + juce::String(clientTimingPort);
        juce::Logger::writeToLog("RaopClient ERROR: " + lastError);
        return false;
    }
    juce::Logger::writeToLog("RaopClient: Successfully bound timing socket to port " + juce::String(clientTimingPort));

    juce::Logger::writeToLog("RaopClient: All UDP sockets created successfully");
    return true;
}
bool RaopClient::selectAvailableClientPorts()
{
    // Try up to 10 sequential ranges starting at 6000 stepping by 10
    for (int base = 6000; base < 6100; base += 10)
    {
        int a = base, c = base + 1, t = base + 2;
        juce::DatagramSocket testA, testC, testT;
        if (testA.bindToPort(a) && testC.bindToPort(c) && testT.bindToPort(t))
        {
            clientAudioPort = a;
            clientControlPort = c;
            clientTimingPort = t;
            juce::Logger::writeToLog("RaopClient: Selected available ports - audio:" + juce::String(a) + " control:" + juce::String(c) + " timing:" + juce::String(t));
            return true;
        }
    }
    juce::Logger::writeToLog("RaopClient: Failed to find available UDP ports");
    return false;
}

void RaopClient::closeUdpSockets()
{
    // Shutdown and recreate sockets to allow rebinding
    if (audioSocket)
    {
        juce::Logger::writeToLog("RaopClient: Shutting down audio socket (port " + juce::String(clientAudioPort) + ")");
        audioSocket->shutdown();
        audioSocket = std::make_unique<juce::DatagramSocket>();
        juce::Logger::writeToLog("RaopClient: Recreated audio socket at " + juce::String::toHexString(reinterpret_cast<uintptr_t>(audioSocket.get())));
    }
    if (controlSocket)
    {
        juce::Logger::writeToLog("RaopClient: Shutting down control socket (port " + juce::String(clientControlPort) + ")");
        controlSocket->shutdown();
        controlSocket = std::make_unique<juce::DatagramSocket>();
        juce::Logger::writeToLog("RaopClient: Recreated control socket at " + juce::String::toHexString(reinterpret_cast<uintptr_t>(controlSocket.get())));
    }
    if (timingSocket)
    {
        juce::Logger::writeToLog("RaopClient: Shutting down timing socket (port " + juce::String(clientTimingPort) + ")");
        timingSocket->shutdown();
        timingSocket = std::make_unique<juce::DatagramSocket>();
        juce::Logger::writeToLog("RaopClient: Recreated timing socket at " + juce::String::toHexString(reinterpret_cast<uintptr_t>(timingSocket.get())));
    }

    // Reset ports to default values for next connection
    clientAudioPort = 6000;
    clientControlPort = 6001;
    clientTimingPort = 6002;
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

// Connection state management
void RaopClient::setConnectionState(ConnectionState newState)
{
    if (connectionState != newState)
    {
        connectionState = newState;
        juce::Logger::writeToLog("RaopClient: Connection state changed to " + getConnectionStateString());
    }
}

juce::String RaopClient::getConnectionStateString() const
{
    switch (connectionState)
    {
        case ConnectionState::Disconnected: return "Disconnected";
        case ConnectionState::Connecting: return "Connecting...";
        case ConnectionState::Connected: return "Connected";
        case ConnectionState::Reconnecting: return "Reconnecting...";
        case ConnectionState::Error: return "Error";
        case ConnectionState::TimedOut: return "Connection Timed Out";
        default: return "Unknown";
    }
}

// Auto-reconnect logic
bool RaopClient::attemptReconnect()
{
    if (!autoReconnectEnabled || reconnectAttempts >= maxReconnectAttempts)
    {
        if (reconnectAttempts >= maxReconnectAttempts)
        {
            logError("Maximum reconnection attempts reached");
            setConnectionState(ConnectionState::Error);
        }
        return false;
    }

    // Exponential backoff: 1s, 2s, 4s, 8s, 16s
    int backoffMs = (1 << reconnectAttempts) * 1000;
    juce::int64 timeSinceLastAttempt = juce::Time::currentTimeMillis() - lastConnectionAttemptTime;

    if (timeSinceLastAttempt < backoffMs)
    {
        return false;  // Too soon to retry
    }

    reconnectAttempts++;
    setConnectionState(ConnectionState::Reconnecting);
    logError("Attempting reconnection (" + juce::String(reconnectAttempts) + "/" +
             juce::String(maxReconnectAttempts) + ")");

    // Try to reconnect
    if (connect(currentDevice))
    {
        logError("Reconnection successful");
        reconnectAttempts = 0;
        return true;
    }

    return false;
}

// Connection health check
bool RaopClient::checkConnection()
{
    if (!connected)
        return false;

    // Check if connection has been idle too long (no successful sends)
    juce::int64 timeSinceLastSend = juce::Time::currentTimeMillis() - lastSuccessfulSendTime;
    if (timeSinceLastSend > 30000)  // 30 seconds
    {
        logError("Connection appears stale (no activity for 30s)");
        setConnectionState(ConnectionState::Error);

        if (autoReconnectEnabled)
        {
            return attemptReconnect();
        }
        return false;
    }

    // Check socket status
    if (!socket || !socket->isConnected())
    {
        logError("Socket disconnected");
        connected = false;
        setConnectionState(ConnectionState::Error);

        if (autoReconnectEnabled)
        {
            return attemptReconnect();
        }
        return false;
    }

    return true;
}

// Error logging
void RaopClient::logError(const juce::String& error)
{
    juce::Logger::writeToLog("RaopClient ERROR: " + error);
    juce::Logger::writeToLog("[RaopClient] " + error);
}

// Wait for socket to be ready
bool RaopClient::waitForSocketReady(juce::StreamingSocket* sock, int timeoutMs)
{
    if (!sock || !sock->isConnected())
        return false;

    juce::int64 startTime = juce::Time::currentTimeMillis();

    while (juce::Time::currentTimeMillis() - startTime < timeoutMs)
    {
        if (sock->waitUntilReady(true, 100) == 1)
            return true;

        juce::Thread::sleep(50);
    }

    return false;
}
