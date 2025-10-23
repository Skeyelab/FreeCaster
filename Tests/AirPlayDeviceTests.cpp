#include <JuceHeader.h>
#include "../Source/Discovery/AirPlayDevice.h"

class AirPlayDeviceTests : public juce::UnitTest
{
public:
    AirPlayDeviceTests() : juce::UnitTest("AirPlayDevice") {}
    
    void runTest() override
    {
        testDeviceConstruction();
        testDeviceValidation();
        testPropertyGettersSetters();
        testEmptyNullValues();
        testPasswordHandling();
        testDeviceEquality();
    }
    
private:
    void testDeviceConstruction()
    {
        beginTest("Device construction with parameters");
        {
            AirPlayDevice device("Living Room", "192.168.1.100", 7000);
            
            expect(device.getDeviceName() == "Living Room", "Device name should be set");
            expect(device.getHostAddress() == "192.168.1.100", "Host address should be set");
            expectEquals(device.getPort(), 7000, "Port should be set");
            expect(device.isValid(), "Device should be valid");
        }
        
        beginTest("Default device construction");
        {
            AirPlayDevice device;
            
            expect(device.getDeviceName().isEmpty(), "Default device name should be empty");
            expect(device.getHostAddress().isEmpty(), "Default host address should be empty");
            expectEquals(device.getPort(), 7000, "Default port should be 7000");
            expect(!device.isValid(), "Default device should not be valid");
        }
    }
    
    void testDeviceValidation()
    {
        beginTest("Device validation - isValid()");
        {
            // Valid device
            AirPlayDevice validDevice("Bedroom", "192.168.1.101", 7000);
            expect(validDevice.isValid(), "Device with name and address should be valid");
            
            // Missing name
            AirPlayDevice noName;
            noName.setHostAddress("192.168.1.102");
            expect(!noName.isValid(), "Device without name should be invalid");
            
            // Missing address
            AirPlayDevice noAddress;
            noAddress.setDeviceName("Kitchen");
            expect(!noAddress.isValid(), "Device without address should be invalid");
            
            // Both missing
            AirPlayDevice empty;
            expect(!empty.isValid(), "Empty device should be invalid");
        }
    }
    
    void testPropertyGettersSetters()
    {
        beginTest("Property getters and setters");
        {
            AirPlayDevice device;
            
            // Test device name
            device.setDeviceName("Test Device");
            expect(device.getDeviceName() == "Test Device", "Device name getter/setter should work");
            
            // Test host address
            device.setHostAddress("10.0.0.1");
            expect(device.getHostAddress() == "10.0.0.1", "Host address getter/setter should work");
            
            // Test port
            device.setPort(5000);
            expectEquals(device.getPort(), 5000, "Port getter/setter should work");
            
            // Test device ID
            device.setDeviceId("ABC123");
            expect(device.getDeviceId() == "ABC123", "Device ID getter/setter should work");
            
            // Device should now be valid
            expect(device.isValid(), "Device should be valid after setting name and address");
        }
    }
    
    void testEmptyNullValues()
    {
        beginTest("Empty and null values");
        {
            AirPlayDevice device;
            
            // Test empty strings
            expect(device.getDeviceName().isEmpty(), "Empty device name should be empty");
            expect(device.getHostAddress().isEmpty(), "Empty host address should be empty");
            expect(device.getDeviceId().isEmpty(), "Empty device ID should be empty");
            expect(device.getPassword().isEmpty(), "Empty password should be empty");
            
            // Set and then clear
            device.setDeviceName("Test");
            expect(device.getDeviceName() == "Test", "Should set name");
            
            device.setDeviceName("");
            expect(device.getDeviceName().isEmpty(), "Should clear name");
            
            // Test with whitespace
            device.setDeviceName("   ");
            expect(device.getDeviceName() == "   ", "Whitespace should be preserved");
        }
    }
    
    void testPasswordHandling()
    {
        beginTest("Password handling");
        {
            AirPlayDevice device("Office", "192.168.1.103", 7000);
            
            // Initially no password
            expect(!device.requiresPassword(), "Initially should not require password");
            expect(device.getPassword().isEmpty(), "Initially password should be empty");
            
            // Set password
            device.setPassword("secret123");
            expect(device.requiresPassword(), "Should require password after setting it");
            expect(device.getPassword() == "secret123", "Password should be set");
            
            // Clear password
            device.setPassword("");
            expect(!device.requiresPassword(), "Should not require password when cleared");
            expect(device.getPassword().isEmpty(), "Password should be empty");
            
            // Manually set requires password flag
            device.setRequiresPassword(true);
            expect(device.requiresPassword(), "Should require password when flag is set");
            
            device.setRequiresPassword(false);
            expect(!device.requiresPassword(), "Should not require password when flag is cleared");
        }
    }
    
    void testDeviceEquality()
    {
        beginTest("Device comparison and equality");
        {
            AirPlayDevice device1("Living Room", "192.168.1.100", 7000);
            AirPlayDevice device2("Living Room", "192.168.1.100", 7000);
            AirPlayDevice device3("Bedroom", "192.168.1.101", 7000);
            
            // Test property equality
            expect(device1.getDeviceName() == device2.getDeviceName(), "Same devices should have same name");
            expect(device1.getHostAddress() == device2.getHostAddress(), "Same devices should have same address");
            expectEquals(device1.getPort(), device2.getPort(), "Same devices should have same port");
            
            // Test difference
            expect(device1.getDeviceName() != device3.getDeviceName(), "Different devices should have different names");
            expect(device1.getHostAddress() != device3.getHostAddress(), "Different devices should have different addresses");
        }
        
        beginTest("Device copying");
        {
            AirPlayDevice original("Original", "1.2.3.4", 8000);
            original.setDeviceId("ID123");
            original.setPassword("pass");
            
            AirPlayDevice copy = original;
            
            expect(copy.getDeviceName() == original.getDeviceName(), "Copied device should have same name");
            expect(copy.getHostAddress() == original.getHostAddress(), "Copied device should have same address");
            expectEquals(copy.getPort(), original.getPort(), "Copied device should have same port");
            expect(copy.getDeviceId() == original.getDeviceId(), "Copied device should have same ID");
            expect(copy.getPassword() == original.getPassword(), "Copied device should have same password");
            expect(copy.requiresPassword() == original.requiresPassword(), "Copied device should have same password requirement");
        }
    }
};

static AirPlayDeviceTests airPlayDeviceTests;
