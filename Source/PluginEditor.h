#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// Professional vertical level meter component
class LevelMeter : public juce::Component
{
public:
    LevelMeter()
    {
        // Make sure the meter is visible
        setOpaque(true);
        setVisible(true);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // ALWAYS draw a visible background for debugging
        g.fillAll(juce::Colour(0xFF3a3a3a));

        // Draw a bright border for visibility
        g.setColour(juce::Colours::cyan);
        g.drawRect(bounds, 3.0f);

        // Draw scale markings on the meter itself
        g.setColour(juce::Colour(0xFF606060));
        g.setFont(8.0f);

        const float dbMarks[] = { 0.0f, -6.0f, -12.0f, -24.0f, -48.0f };
        for (auto db : dbMarks)
        {
            float yPos = dbToY(db, bounds.getHeight());
            g.drawLine(2, yPos, 8, yPos, 1.0f);
        }

        // Calculate meter height
        float meterHeight = bounds.getHeight() * currentLevel;
        float peakHeight = bounds.getHeight() * peakLevel;

        if (meterHeight > 0.0f)
        {
            auto meterBounds = bounds.withTop(bounds.getBottom() - meterHeight).reduced(3.0f);

            // Color gradient based on level
            juce::ColourGradient gradient(
                juce::Colour(0xFF00ff00), meterBounds.getX(), meterBounds.getBottom(),
                juce::Colour(0xFFffff00), meterBounds.getX(), meterBounds.getY(),
                false);

            gradient.addColour(0.7, juce::Colour(0xFF00ff00));  // Green
            gradient.addColour(0.85, juce::Colour(0xFFffff00)); // Yellow
            gradient.addColour(1.0, juce::Colour(0xFFff0000));  // Red

            g.setGradientFill(gradient);
            g.fillRect(meterBounds);
        }

        // Draw peak hold indicator
        if (peakLevel > 0.0f)
        {
            float peakY = bounds.getBottom() - peakHeight;
            g.setColour(peakLevel > 0.9f ? juce::Colours::red : juce::Colours::white);
            g.fillRect(3.0f, peakY - 1, bounds.getWidth() - 6, 2.0f);
        }

        // Debug text
        g.setColour(juce::Colours::white);
        g.setFont(9.0f);
        g.drawText(juce::String(currentLevel, 2), bounds.reduced(2), juce::Justification::centredTop, false);
    }

    void setLevel(float newLevel)
    {
        // Convert to normalized 0-1 range (assuming input is in 0-1 linear amplitude)
        currentLevel = juce::jlimit(0.0f, 1.0f, newLevel);

        // Update peak hold
        if (currentLevel > peakLevel)
        {
            peakLevel = currentLevel;
            peakHoldCounter = 60; // Hold for 60 frames (~1 second at 60Hz)
        }
        else if (peakHoldCounter > 0)
        {
            peakHoldCounter--;
        }
        else
        {
            // Decay peak
            peakLevel *= 0.95f;
            if (peakLevel < 0.001f)
                peakLevel = 0.0f;
        }

        repaint();
    }

    void reset()
    {
        currentLevel = 0.0f;
        peakLevel = 0.0f;
        peakHoldCounter = 0;
        repaint();
    }

private:
    float dbToY(float db, float height) const
    {
        // Map dB to Y position (0dB at top, -inf at bottom)
        float normalized = (db + 60.0f) / 60.0f; // -60dB to 0dB range
        return height * (1.0f - juce::jlimit(0.0f, 1.0f, normalized));
    }

    float currentLevel = 0.0f;
    float peakLevel = 0.0f;
    int peakHoldCounter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeter)
};

//==============================================================================
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
    void updateStatusDisplay();
    void updateBufferHealth();
    void showError(const juce::String& error);
    void showStatus(const juce::String& status);

    AirPlayPluginProcessor& audioProcessor;

    juce::ListBox deviceListBox;
    juce::StringArray deviceNames;
    juce::Array<AirPlayDevice> devices;

    juce::TextButton connectButton;
    juce::TextButton disconnectButton;
    juce::Label statusLabel;
    juce::Label titleLabel;
    juce::Label errorLabel;
    juce::Label bufferHealthLabel;

    // Level meters
    LevelMeter inputMeter;
    LevelMeter outputMeter;
    juce::Label inputMeterLabel;
    juce::Label outputMeterLabel;
    juce::TextButton testAudioButton;
    float testLevel = 0.0f;

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
