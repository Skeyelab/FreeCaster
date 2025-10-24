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
    NSLog(@"AirPlayServiceBrowser: Starting discovery for _raop._tcp.");
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
    NSLog(@"AirPlayServiceBrowser: Found service: %@", [service name]);
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
    NSDictionary<NSString*,NSData*>* txt = [NSNetService dictionaryFromTXTRecordData:[service TXTRecordData]];

    if (hostname && owner)
    {
        juce::String deviceName = juce::String::fromUTF8([name UTF8String]);
        juce::String hostAddress = juce::String::fromUTF8([hostname UTF8String]);

        // Remove trailing dot from hostname
        if (hostAddress.endsWithChar('.'))
            hostAddress = hostAddress.dropLastCharacters(1);

        AirPlayDevice device(deviceName, hostAddress, (int)port);

        // Extract RAOP TXT record public key ("pk") if present
        NSData* pkData = [txt objectForKey:@"pk"];
        if (pkData && [pkData length] > 0)
        {
            juce::String pk = juce::String::fromUTF8((const char*)[pkData bytes], (int)[pkData length]);
            device.setServerPublicKey(pk.trim());
            NSLog(@"RaopClient: Found server public key in TXT record: %s", pk.trim().toRawUTF8());
        }
        else
        {
            NSLog(@"RaopClient: No 'pk' TXT record found for device: %s", [name UTF8String]);
        }
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
