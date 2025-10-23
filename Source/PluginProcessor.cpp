#include "PluginProcessor.h"
#include "PluginEditor.h"

AirPlayPluginProcessor::AirPlayPluginProcessor()
    : AudioProcessor(BusesProperties()
                    .withInput("Input", juce::AudioChannelSet::stereo(), true)
                    .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    deviceDiscovery.startDiscovery();
}

AirPlayPluginProcessor::~AirPlayPluginProcessor()
{
    deviceDiscovery.stopDiscovery();
}

const juce::String AirPlayPluginProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AirPlayPluginProcessor::acceptsMidi() const { return false; }
bool AirPlayPluginProcessor::producesMidi() const { return false; }
bool AirPlayPluginProcessor::isMidiEffect() const { return false; }
double AirPlayPluginProcessor::getTailLengthSeconds() const { return 0.0; }

int AirPlayPluginProcessor::getNumPrograms() { return 1; }
int AirPlayPluginProcessor::getCurrentProgram() { return 0; }
void AirPlayPluginProcessor::setCurrentProgram(int) {}
const juce::String AirPlayPluginProcessor::getProgramName(int) { return {}; }
void AirPlayPluginProcessor::changeProgramName(int, const juce::String&) {}

void AirPlayPluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    airPlayManager.prepare(sampleRate, samplesPerBlock);
}

void AirPlayPluginProcessor::releaseResources()
{
}

bool AirPlayPluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    
    return true;
}

void AirPlayPluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    
    // Calculate input level (RMS)
    float inputRMS = 0.0f;
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        const float* channelData = buffer.getReadPointer(channel);
        float channelRMS = 0.0f;
        
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            channelRMS += channelData[sample] * channelData[sample];
        }
        
        channelRMS = std::sqrt(channelRMS / buffer.getNumSamples());
        inputRMS = juce::jmax(inputRMS, channelRMS);
    }
    
    // Convert RMS to normalized level (0-1) with some headroom
    // RMS of 0.7 (~-3dB) maps to full scale
    float normalizedInput = juce::jlimit(0.0f, 1.0f, inputRMS / 0.7f);
    inputLevel.store(normalizedInput);
    
    // Send to AirPlay if connected
    if (airPlayManager.isConnected())
    {
        airPlayManager.pushAudioData(buffer, buffer.getNumSamples());
        
        // Output level is the same as input for a pass-through plugin
        // (In a real scenario with processing, this might differ)
        outputLevel.store(normalizedInput);
    }
    else
    {
        // Reset output level when not connected
        outputLevel.store(0.0f);
    }
}

bool AirPlayPluginProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* AirPlayPluginProcessor::createEditor()
{
    return new AirPlayPluginEditor(*this);
}

void AirPlayPluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Save plugin state
}

void AirPlayPluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Restore plugin state
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AirPlayPluginProcessor();
}
