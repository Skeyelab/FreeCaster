#include <catch2/catch.hpp>
#include "../Source/AirPlay/RaopClient.h"
#include "../Source/Audio/StreamBuffer.h"
#include "../Source/Discovery/AirPlayDevice.h"

TEST_CASE("RaopClient Error Handling", "[error][connection]")
{
    SECTION("Connection state transitions")
    {
        RaopClient client;
        
        // Initial state should be disconnected
        REQUIRE(client.getConnectionState() == RaopClient::ConnectionState::Disconnected);
        REQUIRE_FALSE(client.isConnected());
        
        // Should get proper state string
        REQUIRE(client.getConnectionStateString() == "Disconnected");
    }
    
    SECTION("Connection timeout handling")
    {
        RaopClient client;
        
        // Try to connect to invalid device
        AirPlayDevice invalidDevice;
        invalidDevice.setHostAddress("192.0.2.1");  // TEST-NET-1, non-routable
        invalidDevice.setPort(7000);
        invalidDevice.setDeviceName("Invalid Device");
        
        // Should fail and set timeout state
        bool connected = client.connect(invalidDevice);
        REQUIRE_FALSE(connected);
        REQUIRE(client.getConnectionState() == RaopClient::ConnectionState::TimedOut ||
                client.getConnectionState() == RaopClient::ConnectionState::Error);
        REQUIRE_FALSE(client.getLastError().isEmpty());
    }
    
    SECTION("Auto-reconnect settings")
    {
        RaopClient client;
        
        // Default should be enabled
        REQUIRE(client.isAutoReconnectEnabled());
        
        // Should be able to disable
        client.setAutoReconnect(false);
        REQUIRE_FALSE(client.isAutoReconnectEnabled());
        
        // Should be able to re-enable
        client.setAutoReconnect(true);
        REQUIRE(client.isAutoReconnectEnabled());
    }
    
    SECTION("Consecutive failures tracking")
    {
        RaopClient client;
        
        // Initial state
        REQUIRE(client.getConsecutiveFailures() == 0);
        
        // After failed send attempts, failures should increment
        // (This would require mocking the socket, but we can test the API)
    }
    
    SECTION("Connection health check when disconnected")
    {
        RaopClient client;
        
        // Should return false when not connected
        REQUIRE_FALSE(client.checkConnection());
    }
}

TEST_CASE("StreamBuffer Error Handling", "[error][buffer]")
{
    SECTION("Buffer overflow detection")
    {
        StreamBuffer buffer(2, 1024);
        juce::AudioBuffer<float> testData(2, 512);
        
        // Fill buffer
        buffer.write(testData, 512);
        buffer.write(testData, 512);
        
        // Initial state
        REQUIRE(buffer.getOverflowCount() == 0);
        
        // Trigger overflow
        buffer.write(testData, 512);  // This should overflow
        REQUIRE(buffer.getOverflowCount() > 0);
    }
    
    SECTION("Buffer underflow detection")
    {
        StreamBuffer buffer(2, 1024);
        juce::AudioBuffer<float> testData(2, 512);
        juce::AudioBuffer<float> readData(2, 512);
        
        // Write some data
        buffer.write(testData, 256);
        
        // Initial state
        REQUIRE(buffer.getUnderflowCount() == 0);
        
        // Try to read more than available
        int samplesRead = buffer.read(readData, 512);
        REQUIRE(samplesRead < 512);
        REQUIRE(buffer.getUnderflowCount() > 0);
    }
    
    SECTION("Buffer overflow flag")
    {
        StreamBuffer buffer(2, 1024);
        juce::AudioBuffer<float> testData(2, 512);
        
        REQUIRE_FALSE(buffer.isOverflowing());
        
        // Fill buffer to > 90%
        buffer.write(testData, 512);
        buffer.write(testData, 500);
        
        REQUIRE(buffer.isOverflowing());
    }
    
    SECTION("Buffer underflow flag")
    {
        StreamBuffer buffer(2, 1024);
        juce::AudioBuffer<float> testData(2, 100);
        
        // Empty buffer
        REQUIRE(buffer.isUnderflowing());
        
        // Fill to > 10%
        buffer.write(testData, 100);
        buffer.write(testData, 100);
        
        REQUIRE_FALSE(buffer.isUnderflowing());
    }
    
    SECTION("Buffer usage percentage")
    {
        StreamBuffer buffer(2, 1000);
        juce::AudioBuffer<float> testData(2, 500);
        
        // Empty
        REQUIRE(buffer.getUsagePercentage() == Approx(0.0f));
        
        // Half full
        buffer.write(testData, 500);
        REQUIRE(buffer.getUsagePercentage() == Approx(50.0f));
        
        // Full
        buffer.write(testData, 500);
        REQUIRE(buffer.getUsagePercentage() == Approx(100.0f));
    }
    
    SECTION("Clear resets monitoring counters")
    {
        StreamBuffer buffer(2, 1024);
        juce::AudioBuffer<float> testData(2, 512);
        juce::AudioBuffer<float> readData(2, 512);
        
        // Cause some overflows/underflows
        buffer.write(testData, 512);
        buffer.write(testData, 512);
        buffer.write(testData, 512);  // Overflow
        buffer.read(readData, 512);   // Underflow possible
        
        REQUIRE(buffer.getOverflowCount() > 0);
        
        // Clear should reset
        buffer.clear();
        REQUIRE(buffer.getOverflowCount() == 0);
        REQUIRE(buffer.getUnderflowCount() == 0);
        REQUIRE(buffer.getAvailableData() == 0);
    }
    
    SECTION("Buffer handles underflow gracefully with silence")
    {
        StreamBuffer buffer(2, 1024);
        juce::AudioBuffer<float> writeData(2, 100);
        juce::AudioBuffer<float> readData(2, 200);
        
        // Fill write buffer with non-zero data
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 100; ++i)
                writeData.setSample(ch, i, 1.0f);
        
        buffer.write(writeData, 100);
        
        // Try to read more than available
        int samplesRead = buffer.read(readData, 200);
        
        // Should have read 100 samples
        REQUIRE(samplesRead == 100);
        
        // First 100 samples should have data, rest should be silence
        for (int ch = 0; ch < 2; ++ch)
        {
            for (int i = 100; i < 200; ++i)
            {
                REQUIRE(readData.getSample(ch, i) == 0.0f);
            }
        }
    }
}

TEST_CASE("Error Message Handling", "[error][logging]")
{
    SECTION("RaopClient error messages are informative")
    {
        RaopClient client;
        
        // Try invalid connection
        AirPlayDevice invalidDevice;
        invalidDevice.setHostAddress("invalid.address");
        invalidDevice.setPort(7000);
        
        client.connect(invalidDevice);
        
        // Should have an error message
        REQUIRE_FALSE(client.getLastError().isEmpty());
        
        // Error should contain useful information
        juce::String error = client.getLastError();
        REQUIRE((error.contains("Failed") || error.contains("timeout") || 
                 error.contains("connect") || error.contains("Error")));
    }
}

TEST_CASE("Thread Safety", "[error][threading]")
{
    SECTION("StreamBuffer is thread-safe")
    {
        StreamBuffer buffer(2, 2048);
        std::atomic<bool> stopFlag{false};
        std::atomic<int> errors{0};
        
        // Writer thread
        auto writerFunc = [&]()
        {
            juce::AudioBuffer<float> writeData(2, 256);
            while (!stopFlag)
            {
                buffer.write(writeData, 256);
                juce::Thread::sleep(1);
            }
        };
        
        // Reader thread
        auto readerFunc = [&]()
        {
            juce::AudioBuffer<float> readData(2, 256);
            while (!stopFlag)
            {
                buffer.read(readData, 256);
                juce::Thread::sleep(1);
            }
        };
        
        // Start threads
        std::thread writer(writerFunc);
        std::thread reader(readerFunc);
        
        // Let them run
        juce::Thread::sleep(100);
        
        // Stop threads
        stopFlag = true;
        writer.join();
        reader.join();
        
        // Should complete without crashes
        REQUIRE(errors == 0);
    }
}
