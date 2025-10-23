#include "ALACEncoderWrapper.h"
#include <cstring>

ALACEncoderWrapper::ALACEncoderWrapper()
{
}

ALACEncoderWrapper::~ALACEncoderWrapper()
{
}

bool ALACEncoderWrapper::initialize(double sampleRate, int numChannels, int samplesPerBlock)
{
    currentSampleRate = static_cast<int>(sampleRate);
    currentNumChannels = numChannels;
    currentFrameSize = samplesPerBlock;
    currentBitDepth = 16; // Using 16-bit for compatibility
    
    // Set up the output format for ALAC
    AudioFormatDescription outputFormat;
    std::memset(&outputFormat, 0, sizeof(AudioFormatDescription));
    
    outputFormat.mSampleRate = sampleRate;
    outputFormat.mFormatID = kALACFormatAppleLossless;
    outputFormat.mFormatFlags = 1; // 1 = 16-bit
    outputFormat.mChannelsPerFrame = numChannels;
    outputFormat.mFramesPerPacket = currentFrameSize;
    
    // Set the frame size before initialization
    encoder.SetFrameSize(currentFrameSize);
    
    // Initialize the encoder
    int32_t status = encoder.InitializeEncoder(outputFormat);
    if (status != 0)
    {
        isInitialized = false;
        return false;
    }
    
    isInitialized = true;
    
    // Pre-allocate buffers
    tempBuffer.resize(currentFrameSize * numChannels);
    outputBuffer.resize(currentFrameSize * numChannels * 4); // Conservative estimate
    
    return true;
}

void ALACEncoderWrapper::convertFloatToInt16(const juce::AudioBuffer<float>& buffer, int numSamples)
{
    int numChannels = buffer.getNumChannels();
    
    // Interleave the audio data and convert to int16
    for (int i = 0; i < numSamples; ++i)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float sample = buffer.getSample(ch, i);
            sample = juce::jlimit(-1.0f, 1.0f, sample);
            tempBuffer[i * numChannels + ch] = static_cast<int16_t>(sample * 32767.0f);
        }
    }
}

juce::MemoryBlock ALACEncoderWrapper::encode(const juce::AudioBuffer<float>& buffer, int numSamples)
{
    juce::MemoryBlock result;
    
    if (!isInitialized)
    {
        return result;
    }
    
    // Convert float audio to int16
    convertFloatToInt16(buffer, numSamples);
    
    // Set up input format
    AudioFormatDescription inputFormat;
    std::memset(&inputFormat, 0, sizeof(AudioFormatDescription));
    
    inputFormat.mSampleRate = currentSampleRate;
    inputFormat.mFormatID = kALACFormatLinearPCM;
    inputFormat.mFormatFlags = kALACFormatFlagIsSignedInteger | kALACFormatFlagsNativeEndian;
    inputFormat.mBytesPerPacket = currentNumChannels * sizeof(int16_t);
    inputFormat.mFramesPerPacket = 1;
    inputFormat.mBytesPerFrame = currentNumChannels * sizeof(int16_t);
    inputFormat.mChannelsPerFrame = currentNumChannels;
    inputFormat.mBitsPerChannel = 16;
    
    // Set up output format
    AudioFormatDescription outputFormat;
    std::memset(&outputFormat, 0, sizeof(AudioFormatDescription));
    
    outputFormat.mSampleRate = currentSampleRate;
    outputFormat.mFormatID = kALACFormatAppleLossless;
    outputFormat.mFormatFlags = 1; // 1 = 16-bit
    outputFormat.mChannelsPerFrame = currentNumChannels;
    outputFormat.mFramesPerPacket = numSamples;
    
    // Encode the data
    int32_t ioNumBytes = numSamples * inputFormat.mBytesPerPacket;
    
    int32_t status = encoder.Encode(
        inputFormat,
        outputFormat,
        reinterpret_cast<unsigned char*>(tempBuffer.data()),
        outputBuffer.data(),
        &ioNumBytes
    );
    
    if (status == 0 && ioNumBytes > 0)
    {
        // Copy the encoded data to the result
        result.setSize(ioNumBytes, false);
        std::memcpy(result.getData(), outputBuffer.data(), ioNumBytes);
    }
    
    return result;
}
