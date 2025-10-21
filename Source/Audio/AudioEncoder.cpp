#include "AudioEncoder.h"

AudioEncoder::AudioEncoder() {}
AudioEncoder::~AudioEncoder() {}

void AudioEncoder::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentSamplesPerBlock = samplesPerBlock;
}

juce::MemoryBlock AudioEncoder::encode(const juce::AudioBuffer<float>& buffer, int numSamples)
{
    switch (currentFormat)
    {
        case Format::PCM_16:
            return encodePCM16(buffer, numSamples);
        case Format::PCM_24:
            return encodePCM24(buffer, numSamples);
        case Format::ALAC:
            return encodeALAC(buffer, numSamples);
        default:
            return encodePCM16(buffer, numSamples);
    }
}

void AudioEncoder::setFormat(Format format)
{
    currentFormat = format;
}

juce::MemoryBlock AudioEncoder::encodePCM16(const juce::AudioBuffer<float>& buffer, int numSamples)
{
    juce::MemoryBlock data;
    int numChannels = buffer.getNumChannels();
    data.setSize(numSamples * numChannels * sizeof(int16_t), true);
    
    int16_t* dest = static_cast<int16_t*>(data.getData());
    
    for (int i = 0; i < numSamples; ++i)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float sample = buffer.getSample(ch, i);
            sample = juce::jlimit(-1.0f, 1.0f, sample);
            *dest++ = static_cast<int16_t>(sample * 32767.0f);
        }
    }
    
    return data;
}

juce::MemoryBlock AudioEncoder::encodePCM24(const juce::AudioBuffer<float>& buffer, int numSamples)
{
    juce::MemoryBlock data;
    int numChannels = buffer.getNumChannels();
    data.setSize(numSamples * numChannels * 3, true);
    
    uint8_t* dest = static_cast<uint8_t*>(data.getData());
    
    for (int i = 0; i < numSamples; ++i)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float sample = buffer.getSample(ch, i);
            sample = juce::jlimit(-1.0f, 1.0f, sample);
            int32_t value = static_cast<int32_t>(sample * 8388607.0f);
            
            *dest++ = static_cast<uint8_t>(value & 0xFF);
            *dest++ = static_cast<uint8_t>((value >> 8) & 0xFF);
            *dest++ = static_cast<uint8_t>((value >> 16) & 0xFF);
        }
    }
    
    return data;
}

juce::MemoryBlock AudioEncoder::encodeALAC(const juce::AudioBuffer<float>& buffer, int numSamples)
{
    // ALAC encoding would require Apple's ALAC encoder library
    // For now, fall back to PCM16
    return encodePCM16(buffer, numSamples);
}
