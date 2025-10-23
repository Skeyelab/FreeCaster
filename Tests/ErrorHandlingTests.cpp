#include <JuceHeader.h>
#include "../Source/AirPlay/RaopClient.h"
#include "../Source/Audio/StreamBuffer.h"
#include "../Source/Discovery/AirPlayDevice.h"
#include <thread>
#include <atomic>

class ErrorHandlingTests : public juce::UnitTest
{
public:
    ErrorHandlingTests() : juce::UnitTest("Error Handling") {}
    
    void runTest() override
    {
        testRaopClientConnectionStates();
        testRaopClientAutoReconnect();
        testRaopClientHealthCheck();
        testStreamBufferOverflowDetection();
        testStreamBufferUnderflowDetection();
        testStreamBufferHealthFlags();
        testStreamBufferUsagePercentage();
        testStreamBufferClearResetsCounters();
        testStreamBufferUnderflowGracefulHandling();
        testErrorMessagesAreInformative();
        testStreamBufferThreadSafety();
    }
    
private:
    void testRaopClientConnectionStates()
    {
        beginTest("RaopClient connection state transitions");
        {
            RaopClient client;
            
            // Initial state should be disconnected
            expect(client.getConnectionState() == RaopClient::ConnectionState::Disconnected, 
                   "Initial state should be disconnected");
            expect(!client.isConnected(), "Should not be connected initially");
            
            // Should get proper state string
            expectEquals(client.getConnectionStateString(), juce::String("Disconnected"), 
                        "Should return correct state string");
        }
    }
    
    void testRaopClientAutoReconnect()
    {
        beginTest("RaopClient auto-reconnect settings");
        {
            RaopClient client;
            
            // Default should be enabled
            expect(client.isAutoReconnectEnabled(), "Auto-reconnect should be enabled by default");
            
            // Should be able to disable
            client.setAutoReconnect(false);
            expect(!client.isAutoReconnectEnabled(), "Should be able to disable auto-reconnect");
            
            // Should be able to re-enable
            client.setAutoReconnect(true);
            expect(client.isAutoReconnectEnabled(), "Should be able to re-enable auto-reconnect");
        }
    }
    
    void testRaopClientHealthCheck()
    {
        beginTest("RaopClient connection health check");
        {
            RaopClient client;
            
            // Should return false when not connected
            expect(!client.checkConnection(), "Health check should return false when disconnected");
            
            // Initial consecutive failures should be 0
            expectEquals(client.getConsecutiveFailures(), 0, "Initial consecutive failures should be 0");
        }
    }
    
    void testStreamBufferOverflowDetection()
    {
        beginTest("StreamBuffer overflow detection");
        {
            StreamBuffer buffer(2, 1024);
            juce::AudioBuffer<float> testData(2, 512);
            
            // Fill buffer
            buffer.write(testData, 512);
            buffer.write(testData, 512);
            
            // Initial state
            expectEquals(buffer.getOverflowCount(), 0, "Initial overflow count should be 0");
            
            // Trigger overflow
            buffer.write(testData, 512);  // This should overflow
            expectGreaterThan(buffer.getOverflowCount(), 0, "Overflow count should increase after overflow");
        }
    }
    
    void testStreamBufferUnderflowDetection()
    {
        beginTest("StreamBuffer underflow detection");
        {
            StreamBuffer buffer(2, 1024);
            juce::AudioBuffer<float> testData(2, 512);
            juce::AudioBuffer<float> readData(2, 512);
            
            // Write some data
            buffer.write(testData, 256);
            
            // Initial state
            expectEquals(buffer.getUnderflowCount(), 0, "Initial underflow count should be 0");
            
            // Try to read more than available
            int samplesRead = buffer.read(readData, 512);
            expectLessThan(samplesRead, 512, "Should read less than requested");
            expectGreaterThan(buffer.getUnderflowCount(), 0, "Underflow count should increase");
        }
    }
    
    void testStreamBufferHealthFlags()
    {
        beginTest("StreamBuffer health flags");
        {
            StreamBuffer buffer(2, 1024);
            juce::AudioBuffer<float> testData(2, 512);
            
            // Empty buffer should be underflowing
            expect(buffer.isUnderflowing(), "Empty buffer should be underflowing");
            expect(!buffer.isOverflowing(), "Empty buffer should not be overflowing");
            
            // Fill buffer to > 90%
            buffer.write(testData, 512);
            buffer.write(testData, 500);
            
            expect(buffer.isOverflowing(), "Buffer > 90% full should be overflowing");
            expect(!buffer.isUnderflowing(), "Buffer > 90% full should not be underflowing");
        }
    }
    
    void testStreamBufferUsagePercentage()
    {
        beginTest("StreamBuffer usage percentage");
        {
            StreamBuffer buffer(2, 1000);
            juce::AudioBuffer<float> testData(2, 500);
            
            // Empty
            expectWithinAbsoluteError(buffer.getUsagePercentage(), 0.0f, 0.1f, 
                                    "Empty buffer should have 0% usage");
            
            // Half full
            buffer.write(testData, 500);
            expectWithinAbsoluteError(buffer.getUsagePercentage(), 50.0f, 0.1f, 
                                    "Half-full buffer should have ~50% usage");
            
            // Full
            buffer.write(testData, 500);
            expectWithinAbsoluteError(buffer.getUsagePercentage(), 100.0f, 0.1f, 
                                    "Full buffer should have ~100% usage");
        }
    }
    
    void testStreamBufferClearResetsCounters()
    {
        beginTest("StreamBuffer clear resets monitoring counters");
        {
            StreamBuffer buffer(2, 1024);
            juce::AudioBuffer<float> testData(2, 512);
            juce::AudioBuffer<float> readData(2, 512);
            
            // Cause some overflows/underflows
            buffer.write(testData, 512);
            buffer.write(testData, 512);
            buffer.write(testData, 512);  // Overflow
            buffer.read(readData, 512);   // Underflow possible
            
            expectGreaterThan(buffer.getOverflowCount(), 0, "Should have overflow count > 0");
            
            // Clear should reset
            buffer.clear();
            expectEquals(buffer.getOverflowCount(), 0, "Overflow count should reset to 0");
            expectEquals(buffer.getUnderflowCount(), 0, "Underflow count should reset to 0");
            expectEquals(buffer.getAvailableData(), 0, "Available data should be 0 after clear");
        }
    }
    
    void testStreamBufferUnderflowGracefulHandling()
    {
        beginTest("StreamBuffer handles underflow gracefully with silence");
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
            expectEquals(samplesRead, 100, "Should read only available samples");
            
            // First 100 samples should have data, rest should be silence
            for (int ch = 0; ch < 2; ++ch)
            {
                for (int i = 100; i < 200; ++i)
                {
                    expectWithinAbsoluteError(readData.getSample(ch, i), 0.0f, 0.0001f, 
                                            "Unfilled samples should be silence");
                }
            }
        }
    }
    
    void testErrorMessagesAreInformative()
    {
        beginTest("Error messages are informative");
        {
            RaopClient client;
            
            // Try invalid connection
            AirPlayDevice invalidDevice;
            invalidDevice.setHostAddress("invalid.address");
            invalidDevice.setPort(7000);
            
            client.connect(invalidDevice);
            
            // Should have an error message
            expect(!client.getLastError().isEmpty(), "Should have an error message");
            
            // Error should contain useful information
            juce::String error = client.getLastError();
            bool hasUsefulInfo = error.contains("Failed") || error.contains("timeout") || 
                                error.contains("connect") || error.contains("Error");
            expect(hasUsefulInfo, "Error message should contain useful information");
        }
    }
    
    void testStreamBufferThreadSafety()
    {
        beginTest("StreamBuffer thread safety");
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
                    try
                    {
                        buffer.write(writeData, 256);
                    }
                    catch (...)
                    {
                        errors++;
                    }
                    juce::Thread::sleep(1);
                }
            };
            
            // Reader thread
            auto readerFunc = [&]()
            {
                juce::AudioBuffer<float> readData(2, 256);
                while (!stopFlag)
                {
                    try
                    {
                        buffer.read(readData, 256);
                    }
                    catch (...)
                    {
                        errors++;
                    }
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
            expectEquals(errors.load(), 0, "Should complete without errors");
        }
    }
};

static ErrorHandlingTests errorHandlingTests;