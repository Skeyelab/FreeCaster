#include "RaopClient.h"

RaopClient::RaopClient()
{
    socket = std::make_unique<juce::StreamingSocket>();
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
    
    if (!socket->connect(device.getHostAddress(), device.getPort(), 5000))
    {
        lastError = "Failed to connect to " + device.getHostAddress();
        return false;
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
        connected = false;
    }
}

bool RaopClient::isConnected() const
{
    return connected;
}

bool RaopClient::sendAudio(const juce::MemoryBlock& audioData, int sampleRate, int channels)
{
    if (!connected)
        return false;
    
    // RTP packet construction would go here
    // This is simplified - real implementation needs RTP headers, timing, etc.
    
    return true;
}

bool RaopClient::sendRtspRequest(const juce::String& method, const juce::String& uri, 
                                  const juce::StringPairArray& headers)
{
    juce::String request = method + " " + uri + " RTSP/1.0\r\n";
    
    for (int i = 0; i < headers.size(); ++i)
        request += headers.getAllKeys()[i] + ": " + headers.getAllValues()[i] + "\r\n";
    
    request += "\r\n";
    
    int sent = socket->write(request.toRawUTF8(), request.length());
    return sent == request.length();
}

bool RaopClient::sendSetup()
{
    juce::StringPairArray headers;
    headers.set("CSeq", "1");
    headers.set("Transport", "RTP/AVP/UDP;unicast;interleaved=0-1;mode=record");
    
    return sendRtspRequest("SETUP", "rtsp://" + currentDevice.getHostAddress() + "/stream", headers);
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
