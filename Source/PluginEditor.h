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
        setOpaque(true);
        setVisible(true);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Draw background
        g.setColour(juce::Colour(0xFF1a1a1a));
        g.fillRoundedRectangle(bounds, 4.0f);

        // Draw scale markings
        g.setColour(juce::Colour(0xFF404040));
        g.setFont(9.0f);
        const float dbMarks[] = { 0.0f, -6.0f, -12.0f, -24.0f, -48.0f };
        for (auto db : dbMarks)
        {
            float yPos = dbToY(db, bounds.getHeight());
            g.drawLine(bounds.getX(), yPos, bounds.getX() + 3, yPos, 1.0f);
            g.drawText(juce::String((int)db), bounds.getX() + bounds.getWidth() + 2, yPos - 6, 30, 12,
                      juce::Justification::left, false);
        }

        // Calculate meter height
        float meterHeight = bounds.getHeight() * currentLevel;
        float peakHeight = bounds.getHeight() * peakLevel;

        if (meterHeight > 0.0f)
        {
            auto meterBounds = bounds.withTop(bounds.getBottom() - meterHeight).reduced(2.0f);
            // Color gradient based on level
            juce::ColourGradient gradient(
                juce::Colour(0xFF00ff00), meterBounds.getX(), meterBounds.getBottom(),
                juce::Colour(0xFFffff00), meterBounds.getX(), meterBounds.getY(),
                false);

            gradient.addColour(0.7, juce::Colour(0xFF00ff00));  // Green
            gradient.addColour(0.85, juce::Colour(0xFFffff00)); // Yellow
            gradient.addColour(1.0, juce::Colour(0xFFff0000));  // Red

            g.setGradientFill(gradient);
            g.fillRoundedRectangle(meterBounds, 2.0f);
        }
        // Draw peak hold indicator
        if (peakLevel > 0.0f)
        {
            float peakY = bounds.getBottom() - peakHeight;
            g.setColour(peakLevel > 0.9f ? juce::Colours::red : juce::Colours::white);
            g.fillRect(bounds.getX() + 1, peakY - 1, bounds.getWidth() - 2, 2.0f);
        }

        // Draw border
        g.setColour(juce::Colour(0xFF505050));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
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
                            public juce::Timer
{
public:
    AirPlayPluginEditor(AirPlayPluginProcessor&);
    ~AirPlayPluginEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void deviceFound(const AirPlayDevice& device);
    void deviceLost(const AirPlayDevice& device);

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
