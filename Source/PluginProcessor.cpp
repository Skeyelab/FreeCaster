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
    
    if (airPlayManager.isConnected())
    {
        airPlayManager.pushAudioData(buffer, buffer.getNumSamples());
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
