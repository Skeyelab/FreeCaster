#include "StreamBuffer.h"

StreamBuffer::StreamBuffer(int numChannels, int bufferSize)
    : buffer(numChannels, bufferSize)
{
    buffer.clear();
}

void StreamBuffer::write(const juce::AudioBuffer<float>& source, int numSamples)
{
    const juce::ScopedLock sl(bufferLock);
    
    // Check for overflow
    if (numStored + numSamples > buffer.getNumSamples())
    {
        overflowCount++;
        DBG("StreamBuffer: Overflow detected, dropping " + 
            juce::String(numStored + numSamples - buffer.getNumSamples()) + " samples");
    }
    
    for (int channel = 0; channel < juce::jmin(source.getNumChannels(), buffer.getNumChannels()); ++channel)
    {
        const float* src = source.getReadPointer(channel);
        float* dest = buffer.getWritePointer(channel);
        int size = buffer.getNumSamples();
        
        for (int i = 0; i < numSamples; ++i)
        {
            dest[(writePos + i) % size] = src[i];
        }
    }
    
    writePos = (writePos + numSamples) % buffer.getNumSamples();
    numStored = juce::jmin(numStored + numSamples, buffer.getNumSamples());
}

int StreamBuffer::read(juce::AudioBuffer<float>& dest, int numSamples)
{
    const juce::ScopedLock sl(bufferLock);
    
    int samplesToRead = juce::jmin(numSamples, numStored);
    
    // Check for underflow
    if (samplesToRead < numSamples && numStored > 0)
    {
        underflowCount++;
        DBG("StreamBuffer: Underflow detected, requested " + juce::String(numSamples) + 
            " but only " + juce::String(samplesToRead) + " available");
    }
    
    for (int channel = 0; channel < juce::jmin(dest.getNumChannels(), buffer.getNumChannels()); ++channel)
    {
        float* destPtr = dest.getWritePointer(channel);
        const float* src = buffer.getReadPointer(channel);
        int size = buffer.getNumSamples();
        
        for (int i = 0; i < samplesToRead; ++i)
        {
            destPtr[i] = src[(readPos + i) % size];
        }
        
        // Fill remaining with silence if underflow
        for (int i = samplesToRead; i < numSamples; ++i)
        {
            destPtr[i] = 0.0f;
        }
    }
    
    readPos = (readPos + samplesToRead) % buffer.getNumSamples();
    numStored -= samplesToRead;
    
    return samplesToRead;
}

int StreamBuffer::getAvailableSpace() const
{
    const juce::ScopedLock sl(bufferLock);
    return buffer.getNumSamples() - numStored;
}

int StreamBuffer::getAvailableData() const
{
    const juce::ScopedLock sl(bufferLock);
    return numStored;
}

void StreamBuffer::clear()
{
    const juce::ScopedLock sl(bufferLock);
    buffer.clear();
    writePos = readPos = numStored = 0;
    overflowCount = 0;
    underflowCount = 0;
}

bool StreamBuffer::isOverflowing() const
{
    const juce::ScopedLock sl(bufferLock);
    return numStored > (buffer.getNumSamples() * 0.9);  // > 90% full
}

bool StreamBuffer::isUnderflowing() const
{
    const juce::ScopedLock sl(bufferLock);
    return numStored < (buffer.getNumSamples() * 0.1);  // < 10% full
}

float StreamBuffer::getUsagePercentage() const
{
    const juce::ScopedLock sl(bufferLock);
    return (float)numStored / (float)buffer.getNumSamples() * 100.0f;
}
