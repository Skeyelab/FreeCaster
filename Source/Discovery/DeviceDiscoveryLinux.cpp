#include "DeviceDiscovery.h"
#include "DeviceDiscoveryPlatform.h"

#if JUCE_LINUX

#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>
#include <thread>
#include <atomic>

class LinuxAvahiImpl
{
public:
    LinuxAvahiImpl(DeviceDiscovery* owner)
        : owner(owner)
        , client(nullptr)
        , browser(nullptr)
        , poll(nullptr)
        , running(false)
        , avahiThread(nullptr)
    {
    }

    ~LinuxAvahiImpl()
    {
        stop();
    }

    void start()
    {
        if (running)
            return;

        running = true;
        avahiThread = std::make_unique<std::thread>([this]() { runAvahiLoop(); });
    }

    void stop()
    {
        running = false;

        if (poll)
        {
            avahi_simple_poll_quit(poll);
        }

        if (avahiThread && avahiThread->joinable())
        {
            avahiThread->join();
        }
        avahiThread.reset();

        cleanup();
    }

private:
    void runAvahiLoop()
    {
        // Create the simple poll object
        poll = avahi_simple_poll_new();
        if (!poll)
        {
            juce::Logger::writeToLog("Failed to create Avahi simple poll");
            return;
        }

        // Create the client
        int error;
        client = avahi_client_new(
            avahi_simple_poll_get(poll),
            (AvahiClientFlags)0,
            clientCallback,
            this,
            &error
        );

        if (!client)
        {
            juce::Logger::writeToLog("Failed to create Avahi client: " + 
                juce::String(avahi_strerror(error)));
            avahi_simple_poll_free(poll);
            poll = nullptr;
            return;
        }

        // Run the main loop
        if (running)
        {
            avahi_simple_poll_loop(poll);
        }

        cleanup();
    }

    void cleanup()
    {
        if (browser)
        {
            avahi_service_browser_free(browser);
            browser = nullptr;
        }

        if (client)
        {
            avahi_client_free(client);
            client = nullptr;
        }

        if (poll)
        {
            avahi_simple_poll_free(poll);
            poll = nullptr;
        }
    }

    static void clientCallback(AvahiClient* client, AvahiClientState state, void* userdata)
    {
        LinuxAvahiImpl* self = static_cast<LinuxAvahiImpl*>(userdata);

        if (state == AVAHI_CLIENT_S_RUNNING)
        {
            // Client is running, create the service browser
            self->createServiceBrowser();
        }
        else if (state == AVAHI_CLIENT_FAILURE)
        {
            juce::Logger::writeToLog("Avahi client failure");
        }
    }

    void createServiceBrowser()
    {
        if (browser)
            return;

        browser = avahi_service_browser_new(
            client,
            AVAHI_IF_UNSPEC,
            AVAHI_PROTO_UNSPEC,
            "_raop._tcp",
            nullptr,
            (AvahiLookupFlags)0,
            browseCallback,
            this
        );

        if (!browser)
        {
            juce::Logger::writeToLog("Failed to create Avahi service browser: " + 
                juce::String(avahi_strerror(avahi_client_errno(client))));
        }
    }

    static void browseCallback(
        AvahiServiceBrowser* browser,
        AvahiIfIndex interface,
        AvahiProtocol protocol,
        AvahiBrowserEvent event,
        const char* name,
        const char* type,
        const char* domain,
        AvahiLookupResultFlags flags,
        void* userdata)
    {
        LinuxAvahiImpl* self = static_cast<LinuxAvahiImpl*>(userdata);

        switch (event)
        {
            case AVAHI_BROWSER_NEW:
                // New service found, resolve it
                avahi_service_resolver_new(
                    self->client,
                    interface,
                    protocol,
                    name,
                    type,
                    domain,
                    AVAHI_PROTO_UNSPEC,
                    (AvahiLookupFlags)0,
                    resolveCallback,
                    userdata
                );
                break;

            case AVAHI_BROWSER_REMOVE:
                // Service removed
                // Could notify owner about device removal
                break;

            case AVAHI_BROWSER_ALL_FOR_NOW:
            case AVAHI_BROWSER_CACHE_EXHAUSTED:
                break;

            case AVAHI_BROWSER_FAILURE:
                juce::Logger::writeToLog("Avahi browser failure");
                break;
        }
    }

    static void resolveCallback(
        AvahiServiceResolver* resolver,
        AvahiIfIndex interface,
        AvahiProtocol protocol,
        AvahiResolverEvent event,
        const char* name,
        const char* type,
        const char* domain,
        const char* hostName,
        const AvahiAddress* address,
        uint16_t port,
        AvahiStringList* txt,
        AvahiLookupResultFlags flags,
        void* userdata)
    {
        LinuxAvahiImpl* self = static_cast<LinuxAvahiImpl*>(userdata);

        if (event == AVAHI_RESOLVER_FOUND)
        {
            // Convert address to string
            char addrStr[AVAHI_ADDRESS_STR_MAX];
            avahi_address_snprint(addrStr, sizeof(addrStr), address);

            juce::String deviceName = juce::String::fromUTF8(name);
            juce::String hostAddress = juce::String::fromUTF8(addrStr);
            int devicePort = port;

            AirPlayDevice device(deviceName, hostAddress, devicePort);
            
            if (self->owner)
            {
                self->owner->addDiscoveredDevice(device);
            }
        }
        else if (event == AVAHI_RESOLVER_FAILURE)
        {
            juce::Logger::writeToLog("Failed to resolve service: " + 
                juce::String::fromUTF8(name));
        }

        // Free the resolver
        avahi_service_resolver_free(resolver);
    }

    DeviceDiscovery* owner;
    AvahiClient* client;
    AvahiServiceBrowser* browser;
    AvahiSimplePoll* poll;
    std::atomic<bool> running;
    std::unique_ptr<std::thread> avahiThread;
};

// PlatformImpl implementation
DeviceDiscovery::PlatformImpl::PlatformImpl(DeviceDiscovery* owner)
{
    impl = new LinuxAvahiImpl(owner);
}

DeviceDiscovery::PlatformImpl::~PlatformImpl()
{
    if (impl)
    {
        delete static_cast<LinuxAvahiImpl*>(impl);
        impl = nullptr;
    }
}

void DeviceDiscovery::PlatformImpl::start()
{
    if (impl)
        static_cast<LinuxAvahiImpl*>(impl)->start();
}

void DeviceDiscovery::PlatformImpl::stop()
{
    if (impl)
        static_cast<LinuxAvahiImpl*>(impl)->stop();
}

#endif // JUCE_LINUX
