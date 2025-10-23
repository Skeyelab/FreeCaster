#include <JuceHeader.h>
#include "../Source/Audio/StreamBuffer.h"
#include <thread>
#include <vector>

class StreamBufferTests : public juce::UnitTest
{
public:
    StreamBufferTests() : juce::UnitTest("StreamBuffer") {}
    
    void runTest() override
    {
        testBasicWriteRead();
        testAvailableSpaceCalculations();
        testBufferOverflow();
        testBufferUnderflow();
        testClearOperation();
        testConcurrentReadWrite();
        testCircularBufferWrapAround();
    }
    
private:
    void testBasicWriteRead()
    {
        beginTest("Basic write and read operations");
        {
            StreamBuffer buffer(2, 1024);
            juce::AudioBuffer<float> writeData(2, 512);
            
            // Fill with test pattern
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 512; ++i)
                    writeData.setSample(ch, i, (float)i / 512.0f);
            
            buffer.write(writeData, 512);
            expectEquals(buffer.getAvailableData(), 512, "Should have 512 samples available");
            
            juce::AudioBuffer<float> readData(2, 512);
            int samplesRead = buffer.read(readData, 512);
            
            expectEquals(samplesRead, 512, "Should read 512 samples");
            expectEquals(buffer.getAvailableData(), 0, "Buffer should be empty after read");
            
            // Verify data integrity
            for (int ch = 0; ch < 2; ++ch)
            {
                for (int i = 0; i < 512; ++i)
                {
                    float expected = (float)i / 512.0f;
                    float actual = readData.getSample(ch, i);
                    expectWithinAbsoluteError(actual, expected, 0.0001f, "Data should match");
                }
            }
        }
    }
    
    void testAvailableSpaceCalculations()
    {
        beginTest("Available space calculations");
        {
            StreamBuffer buffer(2, 1024);
            
            expectEquals(buffer.getAvailableSpace(), 1024, "Initially should have full space");
            expectEquals(buffer.getAvailableData(), 0, "Initially should have no data");
            
            juce::AudioBuffer<float> data(2, 256);
            buffer.write(data, 256);
            
            expectEquals(buffer.getAvailableSpace(), 1024 - 256, "Space should decrease");
            expectEquals(buffer.getAvailableData(), 256, "Data should increase");
            
            buffer.write(data, 256);
            expectEquals(buffer.getAvailableSpace(), 1024 - 512, "Space should continue decreasing");
            expectEquals(buffer.getAvailableData(), 512, "Data should continue increasing");
        }
    }
    
    void testBufferOverflow()
    {
        beginTest("Buffer overflow handling");
        {
            StreamBuffer buffer(2, 512);
            juce::AudioBuffer<float> data(2, 512);
            
            // Fill with test data
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 512; ++i)
                    data.setSample(ch, i, (float)i);
            
            // Write full buffer
            buffer.write(data, 512);
            expectEquals(buffer.getAvailableData(), 512, "Buffer should be full");
            expectEquals(buffer.getAvailableSpace(), 0, "No space should remain");
            
            // Attempt to write more (should handle gracefully)
            juce::AudioBuffer<float> moreData(2, 256);
            buffer.write(moreData, 256);
            
            // Buffer should still contain valid data
            expect(buffer.getAvailableData() <= 512, "Data should not exceed buffer size");
        }
    }
    
    void testBufferUnderflow()
    {
        beginTest("Buffer underflow handling");
        {
            StreamBuffer buffer(2, 1024);
            juce::AudioBuffer<float> writeData(2, 256);
            
            buffer.write(writeData, 256);
            expectEquals(buffer.getAvailableData(), 256, "Should have 256 samples");
            
            // Try to read more than available
            juce::AudioBuffer<float> readData(2, 512);
            int samplesRead = buffer.read(readData, 512);
            
            expectEquals(samplesRead, 256, "Should only read available samples");
            expectEquals(buffer.getAvailableData(), 0, "Buffer should be empty");
        }
    }
    
    void testClearOperation()
    {
        beginTest("Clear operation");
        {
            StreamBuffer buffer(2, 1024);
            juce::AudioBuffer<float> data(2, 512);
            
            buffer.write(data, 512);
            expectEquals(buffer.getAvailableData(), 512, "Should have data");
            
            buffer.clear();
            
            expectEquals(buffer.getAvailableData(), 0, "Buffer should be empty after clear");
            expectEquals(buffer.getAvailableSpace(), 1024, "Full space should be available");
        }
    }
    
    void testConcurrentReadWrite()
    {
        beginTest("Concurrent read/write operations");
        {
            StreamBuffer buffer(2, 4096);
            std::atomic<bool> stopFlag{false};
            std::atomic<int> writeCount{0};
            std::atomic<int> readCount{0};
            
            // Writer thread
            std::thread writer([&]() {
                juce::AudioBuffer<float> data(2, 256);
                for (int i = 0; i < 10; ++i)
                {
                    // Fill with unique pattern
                    for (int ch = 0; ch < 2; ++ch)
                        for (int s = 0; s < 256; ++s)
                            data.setSample(ch, s, float(i * 256 + s));
                    
                    buffer.write(data, 256);
                    writeCount++;
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }
            });
            
            // Reader thread
            std::thread reader([&]() {
                juce::AudioBuffer<float> data(2, 256);
                while (readCount < 10)
                {
                    if (buffer.getAvailableData() >= 256)
                    {
                        buffer.read(data, 256);
                        readCount++;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }
            });
            
            writer.join();
            reader.join();
            
            expectEquals(writeCount.load(), 10, "Should complete all writes");
            expectEquals(readCount.load(), 10, "Should complete all reads");
            expect(buffer.getAvailableData() >= 0, "Buffer state should be valid");
        }
    }
    
    void testCircularBufferWrapAround()
    {
        beginTest("Circular buffer wrap-around");
        {
            StreamBuffer buffer(2, 1024);
            juce::AudioBuffer<float> data(2, 512);
            
            // Fill with pattern
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 512; ++i)
                    data.setSample(ch, i, float(i));
            
            // Write, read, write to force wrap-around
            buffer.write(data, 512);
            
            juce::AudioBuffer<float> readData(2, 512);
            buffer.read(readData, 512);
            
            expectEquals(buffer.getAvailableData(), 0, "Buffer should be empty");
            
            // Write again (should wrap around)
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 512; ++i)
                    data.setSample(ch, i, float(1000 + i));
            
            buffer.write(data, 512);
            expectEquals(buffer.getAvailableData(), 512, "Should have 512 samples after wrap");
            
            buffer.read(readData, 512);
            
            // Verify wrapped data
            for (int ch = 0; ch < 2; ++ch)
            {
                for (int i = 0; i < 512; ++i)
                {
                    expectWithinAbsoluteError(readData.getSample(ch, i), float(1000 + i), 
                                            0.0001f, "Wrapped data should be correct");
                }
            }
        }
    }
};

static StreamBufferTests streamBufferTests;
