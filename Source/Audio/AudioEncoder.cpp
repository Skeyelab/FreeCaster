#include "AudioEncoder.h"

AudioEncoder::AudioEncoder()
{
    alacEncoder = std::make_unique<ALACEncoderWrapper>();
}

AudioEncoder::~AudioEncoder() {}

void AudioEncoder::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentSamplesPerBlock = samplesPerBlock;
    
    // Initialize ALAC encoder if it will be used
    if (currentFormat == Format::ALAC && alacEncoder)
    {
        // Assuming stereo for now - this could be made configurable
        alacInitialized = alacEncoder->initialize(sampleRate, 2, samplesPerBlock);
    }
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
    
    // Re-initialize ALAC encoder if switching to ALAC format
    if (currentFormat == Format::ALAC && alacEncoder)
    {
        alacInitialized = alacEncoder->initialize(currentSampleRate, 2, currentSamplesPerBlock);
    }
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
    // Use Apple's ALAC encoder for lossless compression
    if (alacEncoder && alacInitialized)
    {
        juce::MemoryBlock encoded = alacEncoder->encode(buffer, numSamples);
        
        // If encoding succeeded, return the compressed data
        if (encoded.getSize() > 0)
        {
            return encoded;
        }
    }
    
    // Fall back to PCM16 if ALAC encoding fails or is not initialized
    return encodePCM16(buffer, numSamples);
}
