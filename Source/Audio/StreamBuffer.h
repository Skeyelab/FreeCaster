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
    
private:
    juce::AudioBuffer<float> buffer;
    int writePos = 0;
    int readPos = 0;
    int numStored = 0;
    juce::CriticalSection bufferLock;
};
