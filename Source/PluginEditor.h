#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class AirPlayPluginEditor : public juce::AudioProcessorEditor,
                            public juce::Timer,
                            public DeviceDiscovery::Listener
{
public:
    AirPlayPluginEditor(AirPlayPluginProcessor&);
    ~AirPlayPluginEditor() override;
    
    void paint(juce::Graphics&) override;
    void resized() override;
    
private:
    void timerCallback() override;
    void deviceFound(const AirPlayDevice& device) override;
    void deviceLost(const AirPlayDevice& device) override;
    
    void updateDeviceList();
    void connectButtonClicked();
    void disconnectButtonClicked();
    
    AirPlayPluginProcessor& audioProcessor;
    
    juce::ListBox deviceListBox;
    juce::StringArray deviceNames;
    juce::Array<AirPlayDevice> devices;
    
    juce::TextButton connectButton;
    juce::TextButton disconnectButton;
    juce::Label statusLabel;
    juce::Label titleLabel;
    
    class DeviceListModel : public juce::ListBoxModel
    {
    public:
        DeviceListModel(AirPlayPluginEditor& e) : editor(e) {}
        
        int getNumRows() override
        {
            return editor.deviceNames.size();
        }
        
        void paintListBoxItem(int rowNumber, juce::Graphics& g,
                            int width, int height, bool rowIsSelected) override
        {
            if (rowIsSelected)
                g.fillAll(juce::Colours::lightblue);
            
            g.setColour(juce::Colours::black);
            g.setFont(14.0f);
            
            if (rowNumber < editor.deviceNames.size())
            {
                g.drawText(editor.deviceNames[rowNumber],
                          5, 0, width - 10, height,
                          juce::Justification::centredLeft, true);
            }
        }
        
    private:
        AirPlayPluginEditor& editor;
    };
    
    std::unique_ptr<DeviceListModel> listModel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AirPlayPluginEditor)
};
