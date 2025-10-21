#pragma once
#include <JuceHeader.h>

class AudioEncoder
{
public:
    AudioEncoder();
    ~AudioEncoder();
    
    void prepare(double sampleRate, int samplesPerBlock);
    juce::MemoryBlock encode(const juce::AudioBuffer<float>& buffer, int numSamples);
    
    enum class Format
    {
        PCM_16,
        PCM_24,
        ALAC
    };
    
    void setFormat(Format format);
    Format getFormat() const { return currentFormat; }
    
private:
    Format currentFormat = Format::PCM_16;
    double currentSampleRate = 44100.0;
    int currentSamplesPerBlock = 512;
    
    juce::MemoryBlock encodePCM16(const juce::AudioBuffer<float>& buffer, int numSamples);
    juce::MemoryBlock encodePCM24(const juce::AudioBuffer<float>& buffer, int numSamples);
    juce::MemoryBlock encodeALAC(const juce::AudioBuffer<float>& buffer, int numSamples);
};
