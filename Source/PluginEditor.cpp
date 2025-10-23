#include "PluginProcessor.h"
#include "PluginEditor.h"

AirPlayPluginEditor::AirPlayPluginEditor(AirPlayPluginProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(400, 500);

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

    startTimer(500);  // Update more frequently for better responsiveness
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
    connectButton.setBounds(buttonArea.removeFromLeft(180));
    buttonArea.removeFromLeft(10);
    disconnectButton.setBounds(buttonArea);
}

void AirPlayPluginEditor::timerCallback()
{
    updateStatusDisplay();
    updateBufferHealth();
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
}
