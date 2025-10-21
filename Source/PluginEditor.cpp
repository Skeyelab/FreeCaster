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
    addAndMakeVisible(statusLabel);

    audioProcessor.getDeviceDiscovery().addListener(this);
    updateDeviceList();

    startTimer(1000);
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
    area.removeFromTop(10);

    statusLabel.setBounds(area.removeFromTop(30));
    area.removeFromTop(10);

    deviceListBox.setBounds(area.removeFromTop(300));
    area.removeFromTop(10);

    auto buttonArea = area.removeFromTop(40);
    connectButton.setBounds(buttonArea.removeFromLeft(180));
    buttonArea.removeFromLeft(10);
    disconnectButton.setBounds(buttonArea);
}

void AirPlayPluginEditor::timerCallback()
{
    if (audioProcessor.getAirPlayManager().isConnected())
    {
        statusLabel.setText("Connected to: " +
                          audioProcessor.getAirPlayManager().getConnectedDeviceName(),
                          juce::dontSendNotification);
        connectButton.setEnabled(false);
        disconnectButton.setEnabled(true);
    }
    else
    {
        statusLabel.setText("Not connected", juce::dontSendNotification);
        connectButton.setEnabled(true);
        disconnectButton.setEnabled(false);
    }
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
