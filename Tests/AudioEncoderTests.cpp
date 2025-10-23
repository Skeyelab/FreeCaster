#include <JuceHeader.h>
#include "../Source/Audio/AudioEncoder.h"

class AudioEncoderTests : public juce::UnitTest
{
public:
    AudioEncoderTests() : juce::UnitTest("AudioEncoder") {}
    
    void runTest() override
    {
        testPCM16Encoding();
        testPCM24Encoding();
        testSampleRateHandling();
        testChannelInterleaving();
        testBufferSizeVariations();
        testFloatToIntConversion();
        testFormatSwitching();
    }
    
private:
    void testPCM16Encoding()
    {
        beginTest("PCM 16-bit encoding accuracy");
        {
            AudioEncoder encoder;
            encoder.setFormat(AudioEncoder::Format::PCM_16);
            encoder.prepare(44100.0, 512);
            
            juce::AudioBuffer<float> buffer(2, 256);
            
            // Test with known values
            buffer.setSample(0, 0, 1.0f);   // Max positive
            buffer.setSample(0, 1, -1.0f);  // Max negative
            buffer.setSample(0, 2, 0.0f);   // Zero
            buffer.setSample(0, 3, 0.5f);   // Mid positive
            
            juce::MemoryBlock encoded = encoder.encode(buffer, 256);
            
            // Verify size: 256 samples * 2 channels * 2 bytes
            expectEquals((int)encoded.getSize(), 256 * 2 * 2, "Encoded size should be correct");
            
            // Verify encoded values
            const int16_t* data = static_cast<const int16_t*>(encoded.getData());
            expectEquals((int)data[0], 32767, "Max positive should encode to 32767");
            expectEquals((int)data[2], -32767, "Max negative should encode to -32767");
            expectEquals((int)data[4], 0, "Zero should encode to 0");
            expectWithinAbsoluteError((float)data[6], 16383.5f, 1.0f, "0.5f should encode to ~16383");
        }
    }
    
    void testPCM24Encoding()
    {
        beginTest("PCM 24-bit encoding accuracy");
        {
            AudioEncoder encoder;
            encoder.setFormat(AudioEncoder::Format::PCM_24);
            encoder.prepare(44100.0, 512);
            
            juce::AudioBuffer<float> buffer(2, 128);
            buffer.setSample(0, 0, 1.0f);
            buffer.setSample(0, 1, -1.0f);
            
            juce::MemoryBlock encoded = encoder.encode(buffer, 128);
            
            // Verify size: 128 samples * 2 channels * 3 bytes
            expectEquals((int)encoded.getSize(), 128 * 2 * 3, "Encoded size should be correct for PCM24");
            
            // Verify first sample (1.0f -> 8388607)
            const uint8_t* data = static_cast<const uint8_t*>(encoded.getData());
            int32_t value = data[0] | (data[1] << 8) | (data[2] << 16);
            if (value & 0x800000) value |= 0xFF000000;  // Sign extend
            
            expectWithinAbsoluteError((float)value, 8388607.0f, 1.0f, "Max value should be ~8388607");
        }
    }
    
    void testSampleRateHandling()
    {
        beginTest("Sample rate handling (44.1kHz and 48kHz)");
        {
            AudioEncoder encoder;
            encoder.setFormat(AudioEncoder::Format::PCM_16);
            
            // Test 44.1kHz
            encoder.prepare(44100.0, 512);
            juce::AudioBuffer<float> buffer441(2, 512);
            juce::MemoryBlock encoded441 = encoder.encode(buffer441, 512);
            expect(encoded441.getSize() > 0, "Should encode 44.1kHz audio");
            
            // Test 48kHz
            encoder.prepare(48000.0, 512);
            juce::AudioBuffer<float> buffer48(2, 512);
            juce::MemoryBlock encoded48 = encoder.encode(buffer48, 512);
            expect(encoded48.getSize() > 0, "Should encode 48kHz audio");
            
            // Sizes should be the same for same buffer size
            expectEquals((int)encoded441.getSize(), (int)encoded48.getSize(), 
                        "Encoded sizes should match for same buffer size");
        }
    }
    
    void testChannelInterleaving()
    {
        beginTest("Channel interleaving (stereo)");
        {
            AudioEncoder encoder;
            encoder.setFormat(AudioEncoder::Format::PCM_16);
            encoder.prepare(44100.0, 512);
            
            juce::AudioBuffer<float> buffer(2, 4);
            
            // Set distinct values per channel
            buffer.setSample(0, 0, 0.1f);  // Left channel, sample 0
            buffer.setSample(1, 0, 0.2f);  // Right channel, sample 0
            buffer.setSample(0, 1, 0.3f);  // Left channel, sample 1
            buffer.setSample(1, 1, 0.4f);  // Right channel, sample 1
            
            juce::MemoryBlock encoded = encoder.encode(buffer, 4);
            const int16_t* data = static_cast<const int16_t*>(encoded.getData());
            
            // Verify interleaving: L0, R0, L1, R1, ...
            float tolerance = 100.0f;
            expectWithinAbsoluteError((float)data[0], 0.1f * 32767, tolerance, "L0 should be interleaved first");
            expectWithinAbsoluteError((float)data[1], 0.2f * 32767, tolerance, "R0 should be interleaved second");
            expectWithinAbsoluteError((float)data[2], 0.3f * 32767, tolerance, "L1 should be interleaved third");
            expectWithinAbsoluteError((float)data[3], 0.4f * 32767, tolerance, "R1 should be interleaved fourth");
        }
    }
    
    void testBufferSizeVariations()
    {
        beginTest("Buffer size variations");
        {
            AudioEncoder encoder;
            encoder.setFormat(AudioEncoder::Format::PCM_16);
            encoder.prepare(44100.0, 512);
            
            // Test various buffer sizes
            int sizes[] = {64, 128, 256, 512, 1024, 2048};
            
            for (int size : sizes)
            {
                juce::AudioBuffer<float> buffer(2, size);
                buffer.clear();
                
                juce::MemoryBlock encoded = encoder.encode(buffer, size);
                
                expectEquals((int)encoded.getSize(), size * 2 * 2, 
                            "Size " + juce::String(size) + " should encode correctly");
            }
        }
    }
    
    void testFloatToIntConversion()
    {
        beginTest("Float to int16 conversion precision");
        {
            AudioEncoder encoder;
            encoder.setFormat(AudioEncoder::Format::PCM_16);
            encoder.prepare(44100.0, 512);
            
            juce::AudioBuffer<float> buffer(2, 10);
            buffer.clear();
            
            // Test various float values
            float testValues[] = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f, -0.25f, -0.5f, -0.75f, -1.0f};
            
            // Fill both channels with the same test data
            for (int i = 0; i < 9; ++i)
            {
                buffer.setSample(0, i, testValues[i]);
                buffer.setSample(1, i, testValues[i]);
            }
            
            juce::MemoryBlock encoded = encoder.encode(buffer, 10);
            const int16_t* data = static_cast<const int16_t*>(encoded.getData());
            
            // Verify conversion accuracy (allowing small rounding errors)
            // Data is interleaved: [L0, R0, L1, R1, ...]
            for (int i = 0; i < 9; ++i)
            {
                float expected = testValues[i] * 32767.0f;
                float actualLeft = (float)data[i * 2];      // Left channel
                float actualRight = (float)data[i * 2 + 1]; // Right channel
                
                expectWithinAbsoluteError(actualLeft, expected, 1.0f, 
                    "Left channel value " + juce::String(testValues[i]) + " should convert accurately");
                expectWithinAbsoluteError(actualRight, expected, 1.0f, 
                    "Right channel value " + juce::String(testValues[i]) + " should convert accurately");
            }
        }
    }
    
    void testFormatSwitching()
    {
        beginTest("Format switching");
        {
            AudioEncoder encoder;
            juce::AudioBuffer<float> buffer(2, 256);
            buffer.clear();
            
            // Start with PCM16
            encoder.setFormat(AudioEncoder::Format::PCM_16);
            expectEquals((int)encoder.getFormat(), (int)AudioEncoder::Format::PCM_16, 
                        "Format should be PCM_16");
            
            juce::MemoryBlock encoded16 = encoder.encode(buffer, 256);
            expectEquals((int)encoded16.getSize(), 256 * 2 * 2, "PCM16 size should be correct");
            
            // Switch to PCM24
            encoder.setFormat(AudioEncoder::Format::PCM_24);
            expectEquals((int)encoder.getFormat(), (int)AudioEncoder::Format::PCM_24, 
                        "Format should be PCM_24");
            
            juce::MemoryBlock encoded24 = encoder.encode(buffer, 256);
            expectEquals((int)encoded24.getSize(), 256 * 2 * 3, "PCM24 size should be correct");
            
            // Switch to ALAC (currently falls back to PCM16)
            encoder.setFormat(AudioEncoder::Format::ALAC);
            expectEquals((int)encoder.getFormat(), (int)AudioEncoder::Format::ALAC, 
                        "Format should be ALAC");
            
            juce::MemoryBlock encodedALAC = encoder.encode(buffer, 256);
            expect(encodedALAC.getSize() > 0, "ALAC should produce output (even if fallback)");
        }
    }
};

static AudioEncoderTests audioEncoderTests;
