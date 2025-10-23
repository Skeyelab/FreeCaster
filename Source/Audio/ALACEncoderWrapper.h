#pragma once
#include "ALAC/ALACEncoder.h"
#include "ALAC/ALACAudioTypes.h"
#include <JuceHeader.h>

class ALACEncoderWrapper
{
public:
    ALACEncoderWrapper();
    ~ALACEncoderWrapper();
    
    bool initialize(double sampleRate, int numChannels, int samplesPerBlock);
    juce::MemoryBlock encode(const juce::AudioBuffer<float>& buffer, int numSamples);
    
private:
    ALACEncoder encoder;
    bool isInitialized = false;
    
    int currentSampleRate = 44100;
    int currentNumChannels = 2;
    int currentFrameSize = 4096;
    int currentBitDepth = 16;
    
    std::vector<int16_t> tempBuffer;
    std::vector<uint8_t> outputBuffer;
    
    void convertFloatToInt16(const juce::AudioBuffer<float>& buffer, int numSamples);
};
