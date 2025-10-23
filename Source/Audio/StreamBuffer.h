#pragma once
#include <JuceHeader.h>

class StreamBuffer
{
public:
    StreamBuffer(int numChannels = 2, int bufferSize = 8192);
    
    void write(const juce::AudioBuffer<float>& source, int numSamples);
    int read(juce::AudioBuffer<float>& dest, int numSamples);
    
    int getAvailableSpace() const;
    int getAvailableData() const;
    void clear();
    
    // Buffer health monitoring
    bool isOverflowing() const;
    bool isUnderflowing() const;
    float getUsagePercentage() const;
    int getOverflowCount() const { return overflowCount; }
    int getUnderflowCount() const { return underflowCount; }
    
private:
    juce::AudioBuffer<float> buffer;
    int writePos = 0;
    int readPos = 0;
    int numStored = 0;
    juce::CriticalSection bufferLock;
    
    // Monitoring
    std::atomic<int> overflowCount{0};
    std::atomic<int> underflowCount{0};
};
