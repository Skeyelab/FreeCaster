#include "PluginProcessor.h"
#include "PluginEditor.h"

AirPlayPluginEditor::AirPlayPluginEditor(AirPlayPluginProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Set a reasonable size that works for both standalone and plugin contexts
    setSize(600, 500);
    setResizable(true, true);
    setResizeLimits(400, 400, 800, 600);

    titleLabel.setText("FreeCaster", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(24.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    listModel = std::make_unique<DeviceListModel>(*this);
    deviceListBox.setModel(listModel.get());
    deviceListBox.setRowHeight(30);
    addAndMakeVisible(deviceListBox);

    connectButton.setButtonText("Connect");
    connectButton.onClick = [this] { connectButtonClicked(); };
    addAndMakeVisible(connectButton);

    disconnectButton.setButtonText("Disconnect");
    disconnectButton.onClick = [this] { disconnectButtonClicked(); };
    disconnectButton.setEnabled(false);
    addAndMakeVisible(disconnectButton);

    statusLabel.setText("Not connected", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(statusLabel);

    errorLabel.setText("", juce::dontSendNotification);
    errorLabel.setJustificationType(juce::Justification::centred);
    errorLabel.setColour(juce::Label::textColourId, juce::Colours::red);
    errorLabel.setFont(juce::Font(12.0f));
    addAndMakeVisible(errorLabel);

    bufferHealthLabel.setText("Buffer: Idle", juce::dontSendNotification);
    bufferHealthLabel.setJustificationType(juce::Justification::centred);
    bufferHealthLabel.setFont(juce::Font(11.0f));
    bufferHealthLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(bufferHealthLabel);

    // Set up level meters
    addAndMakeVisible(inputMeter);
    addAndMakeVisible(outputMeter);

    // Ensure meters are visible
    inputMeter.setVisible(true);
    outputMeter.setVisible(true);

    inputMeterLabel.setText("Input", juce::dontSendNotification);
    inputMeterLabel.setJustificationType(juce::Justification::centred);
    inputMeterLabel.setFont(juce::Font(11.0f));
    addAndMakeVisible(inputMeterLabel);

    outputMeterLabel.setText("Output", juce::dontSendNotification);
    outputMeterLabel.setJustificationType(juce::Justification::centred);
    outputMeterLabel.setFont(juce::Font(11.0f));
    addAndMakeVisible(outputMeterLabel);

    // Test audio button
    testAudioButton.setButtonText("Test Meters");
    testAudioButton.onClick = [this] {
        testLevel = (testLevel > 0.0f) ? 0.0f : 0.75f;
    };
    addAndMakeVisible(testAudioButton);

    // Set up error callbacks
    audioProcessor.getAirPlayManager().onError = [this](const juce::String& error)
    {
        showError(error);
    };

    audioProcessor.getAirPlayManager().onStatusChange = [this](const juce::String& status)
    {
        showStatus(status);
    };

    audioProcessor.getDeviceDiscovery().addListener(this);
    updateDeviceList();

    startTimer(16);  // ~60Hz for smooth meter updates
}

AirPlayPluginEditor::~AirPlayPluginEditor()
{
    audioProcessor.getDeviceDiscovery().removeListener(this);
}

void AirPlayPluginEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void AirPlayPluginEditor::resized()
{
    auto area = getLocalBounds().reduced(10);

    // Reserve space for meters on the right
    auto metersArea = area.removeFromRight(100);
    area.removeFromRight(10); // Gap

    titleLabel.setBounds(area.removeFromTop(40));
    area.removeFromTop(5);

    statusLabel.setBounds(area.removeFromTop(25));
    area.removeFromTop(3);

    errorLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(3);

    bufferHealthLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(10);

    deviceListBox.setBounds(area.removeFromTop(280));
    area.removeFromTop(10);

    auto buttonArea = area.removeFromTop(40);
    connectButton.setBounds(buttonArea.removeFromLeft(110));
    buttonArea.removeFromLeft(5);
    disconnectButton.setBounds(buttonArea.removeFromLeft(110));
    buttonArea.removeFromLeft(5);
    testAudioButton.setBounds(buttonArea);

    // Layout meters vertically on the right side
    metersArea.removeFromTop(40); // Align with title

    auto inputMeterArea = metersArea.removeFromLeft(40);
    metersArea.removeFromLeft(10);
    auto outputMeterArea = metersArea.removeFromLeft(40);

    inputMeterLabel.setBounds(inputMeterArea.removeFromTop(20));
    inputMeter.setBounds(inputMeterArea); // Use remaining height

    outputMeterLabel.setBounds(outputMeterArea.removeFromTop(20));
    outputMeter.setBounds(outputMeterArea); // Use remaining height

    // Debug: Log meter bounds
    DBG("=== RESIZE DEBUG ===");
    DBG("Window size: " + juce::String(getWidth()) + "x" + juce::String(getHeight()));
    DBG("Input meter bounds: " + inputMeter.getBounds().toString());
    DBG("Output meter bounds: " + outputMeter.getBounds().toString());
    DBG("Input meter visible: " + juce::String(inputMeter.isVisible()));
    DBG("Output meter visible: " + juce::String(outputMeter.isVisible()));
    DBG("Meters area initial: " + juce::String(metersArea.getWidth()) + "x" + juce::String(metersArea.getHeight()));
}

void AirPlayPluginEditor::timerCallback()
{
    updateStatusDisplay();
    updateBufferHealth();

    // Update level meters
    float inputLevel = (testLevel > 0.0f) ? testLevel : audioProcessor.getInputLevel();
    float outputLevel = (testLevel > 0.0f) ? testLevel : audioProcessor.getOutputLevel();

    inputMeter.setLevel(inputLevel);
    outputMeter.setLevel(outputLevel);

    // Debug: Log levels occasionally
    static int debugCounter = 0;
    if (++debugCounter % 60 == 0) // Log every second
    {
        DBG("Input level: " + juce::String(inputLevel) + ", Output level: " + juce::String(outputLevel));
        DBG("Test level: " + juce::String(testLevel));
    }
}

void AirPlayPluginEditor::updateStatusDisplay()
{
    auto& manager = audioProcessor.getAirPlayManager();

    if (manager.isConnected())
    {
        statusLabel.setText(manager.getConnectionStatus(), juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
        connectButton.setEnabled(false);
        disconnectButton.setEnabled(true);

        // Clear error if connected successfully
        juce::String lastError = manager.getLastError();
        if (lastError.isEmpty())
            errorLabel.setText("", juce::dontSendNotification);
    }
    else
    {
        statusLabel.setText(manager.getConnectionStatus(), juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        connectButton.setEnabled(true);
        disconnectButton.setEnabled(false);

        // Show error if present
        juce::String lastError = manager.getLastError();
        if (lastError.isNotEmpty())
        {
            errorLabel.setText("Error: " + lastError, juce::dontSendNotification);
        }
    }
}

void AirPlayPluginEditor::updateBufferHealth()
{
    // This would require access to the stream buffer
    // For now, show a simple status
    if (audioProcessor.getAirPlayManager().isConnected())
    {
        bufferHealthLabel.setText("Buffer: Streaming", juce::dontSendNotification);
        bufferHealthLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
    }
    else
    {
        bufferHealthLabel.setText("Buffer: Idle", juce::dontSendNotification);
        bufferHealthLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    }
}

void AirPlayPluginEditor::showError(const juce::String& error)
{
    errorLabel.setText("⚠ " + error, juce::dontSendNotification);
    DBG("GUI Error: " + error);
}

void AirPlayPluginEditor::showStatus(const juce::String& status)
{
    statusLabel.setText(status, juce::dontSendNotification);
    if (status.contains("Connected"))
    {
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
        errorLabel.setText("", juce::dontSendNotification);
    }
    else if (status.contains("Error") || status.contains("Failed"))
    {
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::red);
    }
    else
    {
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    }
    DBG("GUI Status: " + status);
}

void AirPlayPluginEditor::deviceFound(const AirPlayDevice& device)
{
    updateDeviceList();
}

void AirPlayPluginEditor::deviceLost(const AirPlayDevice& device)
{
    updateDeviceList();
}

void AirPlayPluginEditor::updateDeviceList()
{
    devices = audioProcessor.getDeviceDiscovery().getDiscoveredDevices();
    deviceNames.clear();

    for (const auto& device : devices)
    {
        deviceNames.add(device.getDeviceName() + " (" + device.getHostAddress() + ")");
    }

    deviceListBox.updateContent();
}

void AirPlayPluginEditor::connectButtonClicked()
{
    int selectedRow = deviceListBox.getSelectedRow();
    if (selectedRow >= 0 && selectedRow < devices.size())
    {
        audioProcessor.getAirPlayManager().connectToDevice(devices[selectedRow]);
    }
}

void AirPlayPluginEditor::disconnectButtonClicked()
{
    audioProcessor.getAirPlayManager().disconnectFromDevice();

    // Reset meters when disconnected
    outputMeter.reset();
}
