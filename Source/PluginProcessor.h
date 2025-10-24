#pragma once

#include <JuceHeader.h>
#include "AirPlay/AirPlayManager.h"

class AirPlayPluginProcessor : public juce::AudioProcessor
{
public:
    AirPlayPluginProcessor();
    ~AirPlayPluginProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    AirPlayManager& getAirPlayManager() { return airPlayManager; }
    DeviceDiscovery& getDeviceDiscovery() { return deviceDiscovery; }

    // Level meter accessors
    float getInputLevel() const { return inputLevel.load(); }
    float getOutputLevel() const { return outputLevel.load(); }

private:
    AirPlayManager airPlayManager;
    DeviceDiscovery deviceDiscovery;

    // Level tracking for meters (thread-safe)
    std::atomic<float> inputLevel{ 0.0f };
    std::atomic<float> outputLevel{ 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AirPlayPluginProcessor)
};
