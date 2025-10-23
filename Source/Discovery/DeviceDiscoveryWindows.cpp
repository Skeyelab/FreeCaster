#include "DeviceDiscovery.h"
#include "DeviceDiscoveryPlatform.h"

#if JUCE_WINDOWS

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windns.h>

#pragma comment(lib, "dnsapi.lib")
#pragma comment(lib, "ws2_32.lib")

class WindowsDNSSDImpl
{
public:
    WindowsDNSSDImpl(DeviceDiscovery* owner)
        : owner(owner)
        , running(false)
        , discoveryThread(nullptr)
    {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
    }

    ~WindowsDNSSDImpl()
    {
        stop();
        WSACleanup();
    }

    void start()
    {
        if (running)
            return;

        running = true;
        discoveryThread = std::make_unique<std::thread>([this]() { discoveryLoop(); });
    }

    void stop()
    {
        running = false;
        if (discoveryThread && discoveryThread->joinable())
        {
            discoveryThread->join();
        }
        discoveryThread.reset();
    }

private:
    void discoveryLoop()
    {
        while (running)
        {
            performDiscovery();
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    void performDiscovery()
    {
        // Query for _raop._tcp.local PTR records
        PDNS_RECORD pDnsRecord = nullptr;
        DNS_STATUS status = DnsQuery_UTF8(
            "_raop._tcp.local",
            DNS_TYPE_PTR,
            DNS_QUERY_STANDARD,
            nullptr,
            &pDnsRecord,
            nullptr
        );

        if (status == 0 && pDnsRecord)
        {
            PDNS_RECORD pRecord = pDnsRecord;
            
            while (pRecord)
            {
                if (pRecord->wType == DNS_TYPE_PTR)
                {
                    // PTR record gives us service instance name
                    const char* instanceName = pRecord->Data.PTR.pNameHost;
                    
                    if (instanceName)
                    {
                        // Resolve the SRV record to get hostname and port
                        resolveSRVRecord(instanceName);
                    }
                }
                pRecord = pRecord->pNext;
            }
            
            DnsRecordListFree(pDnsRecord, DnsFreeRecordList);
        }
    }

    void resolveSRVRecord(const char* serviceName)
    {
        PDNS_RECORD pDnsRecord = nullptr;
        DNS_STATUS status = DnsQuery_UTF8(
            serviceName,
            DNS_TYPE_SRV,
            DNS_QUERY_STANDARD,
            nullptr,
            &pDnsRecord,
            nullptr
        );

        if (status == 0 && pDnsRecord)
        {
            PDNS_RECORD pRecord = pDnsRecord;
            
            while (pRecord)
            {
                if (pRecord->wType == DNS_TYPE_SRV)
                {
                    juce::String hostname = juce::String::fromUTF8(pRecord->Data.SRV.pNameTarget);
                    int port = pRecord->Data.SRV.wPort;
                    
                    // Extract device name from service name
                    // Format: "DeviceName._raop._tcp.local"
                    juce::String deviceName = juce::String::fromUTF8(serviceName);
                    int dotPos = deviceName.indexOf("._raop");
                    if (dotPos > 0)
                    {
                        deviceName = deviceName.substring(0, dotPos);
                    }
                    
                    // Remove trailing dot from hostname
                    if (hostname.endsWithChar('.'))
                        hostname = hostname.dropLastCharacters(1);
                    
                    // Resolve hostname to IP address
                    resolveHostname(deviceName, hostname, port);
                }
                pRecord = pRecord->pNext;
            }
            
            DnsRecordListFree(pDnsRecord, DnsFreeRecordList);
        }
    }

    void resolveHostname(const juce::String& deviceName, const juce::String& hostname, int port)
    {
        struct addrinfo hints = {0};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        struct addrinfo* result = nullptr;
        int ret = getaddrinfo(hostname.toRawUTF8(), nullptr, &hints, &result);

        if (ret == 0 && result)
        {
            // Use the first address
            char ipStr[INET6_ADDRSTRLEN];
            void* addr;

            if (result->ai_family == AF_INET)
            {
                struct sockaddr_in* ipv4 = (struct sockaddr_in*)result->ai_addr;
                addr = &(ipv4->sin_addr);
            }
            else
            {
                struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)result->ai_addr;
                addr = &(ipv6->sin6_addr);
            }

            inet_ntop(result->ai_family, addr, ipStr, sizeof(ipStr));
            
            juce::String hostAddress = juce::String::fromUTF8(ipStr);
            AirPlayDevice device(deviceName, hostAddress, port);
            
            if (owner)
            {
                owner->addDiscoveredDevice(device);
            }

            freeaddrinfo(result);
        }
    }

    DeviceDiscovery* owner;
    std::atomic<bool> running;
    std::unique_ptr<std::thread> discoveryThread;
};

// PlatformImpl implementation
DeviceDiscovery::PlatformImpl::PlatformImpl(DeviceDiscovery* owner)
{
    impl = new WindowsDNSSDImpl(owner);
}

DeviceDiscovery::PlatformImpl::~PlatformImpl()
{
    if (impl)
    {
        delete static_cast<WindowsDNSSDImpl*>(impl);
        impl = nullptr;
    }
}

void DeviceDiscovery::PlatformImpl::start()
{
    if (impl)
        static_cast<WindowsDNSSDImpl*>(impl)->start();
}

void DeviceDiscovery::PlatformImpl::stop()
{
    if (impl)
        static_cast<WindowsDNSSDImpl*>(impl)->stop();
}

#endif // JUCE_WINDOWS
