#include "DeviceDiscovery.h"
#include "DeviceDiscoveryPlatform.h"

#if JUCE_MAC

#import <Foundation/Foundation.h>

@interface AirPlayServiceBrowser : NSObject <NSNetServiceBrowserDelegate, NSNetServiceDelegate>
{
    NSNetServiceBrowser* browser;
    NSMutableArray* services;
    DeviceDiscovery* owner;
}

- (id)initWithOwner:(DeviceDiscovery*)discoveryOwner;
- (void)start;
- (void)stop;

@end

@implementation AirPlayServiceBrowser

- (id)initWithOwner:(DeviceDiscovery*)discoveryOwner
{
    self = [super init];
    if (self)
    {
        owner = discoveryOwner;
        services = [[NSMutableArray alloc] init];
        browser = [[NSNetServiceBrowser alloc] init];
        [browser setDelegate:self];
    }
    return self;
}

- (void)start
{
    [browser searchForServicesOfType:@"_raop._tcp." inDomain:@"local."];
}

- (void)stop
{
    [browser stop];
}

- (void)netServiceBrowser:(NSNetServiceBrowser*)browser
           didFindService:(NSNetService*)service
               moreComing:(BOOL)moreComing
{
    [services addObject:service];
    [service setDelegate:self];
    [service resolveWithTimeout:5.0];
}

- (void)netServiceBrowser:(NSNetServiceBrowser*)browser
         didRemoveService:(NSNetService*)service
               moreComing:(BOOL)moreComing
{
    [services removeObject:service];
    // Notify removal
}

- (void)netServiceDidResolveAddress:(NSNetService*)service
{
    NSString* hostname = [service hostName];
    NSInteger port = [service port];
    NSString* name = [service name];

    if (hostname && owner)
    {
        juce::String deviceName = juce::String::fromUTF8([name UTF8String]);
        juce::String hostAddress = juce::String::fromUTF8([hostname UTF8String]);

        // Remove trailing dot from hostname
        if (hostAddress.endsWithChar('.'))
            hostAddress = hostAddress.dropLastCharacters(1);

        AirPlayDevice device(deviceName, hostAddress, (int)port);
        owner->addDiscoveredDevice(device);
    }
}

- (void)netService:(NSNetService*)service didNotResolve:(NSDictionary*)errorDict
{
    NSLog(@"Failed to resolve service: %@", [service name]);
}

- (void)dealloc
{
    [browser stop];
    [browser release];
    [services release];
    [super dealloc];
}

@end

// C++ bridge implementation
DeviceDiscovery::PlatformImpl::PlatformImpl(DeviceDiscovery* owner)
{
    @autoreleasepool
    {
        browser = [[AirPlayServiceBrowser alloc] initWithOwner:owner];
    }
}

DeviceDiscovery::PlatformImpl::~PlatformImpl()
{
    @autoreleasepool
    {
        AirPlayServiceBrowser* b = (__bridge AirPlayServiceBrowser*)browser;
        [b stop];
        [b release];
    }
}

void DeviceDiscovery::PlatformImpl::start()
{
    @autoreleasepool
    {
        [(__bridge AirPlayServiceBrowser*)browser start];
    }
}

void DeviceDiscovery::PlatformImpl::stop()
{
    @autoreleasepool
    {
        [(__bridge AirPlayServiceBrowser*)browser stop];
    }
}

#endif
