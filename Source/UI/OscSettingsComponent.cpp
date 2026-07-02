#include "OscSettingsComponent.h"

#include "Core/OscAddress.h"

namespace conduit
{

OscSettingsComponent::OscSettingsComponent (OscSendSettings& settingsToUse,
                                            OscController& oscControllerToUse)
    : settings (settingsToUse),
      oscController (oscControllerToUse)
{
    receiveHeader.setText ("Empfang", juce::dontSendNotification);
    receiveHeader.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));
    addAndMakeVisible (receiveHeader);

    receiveStatusLabel.setColour (juce::Label::textColourId,
                                  juce::Colours::white.withAlpha (0.7f));
    addAndMakeVisible (receiveStatusLabel);

    sendHeader.setText ("Senden (Parameter-Feedback)", juce::dontSendNotification);
    sendHeader.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));
    addAndMakeVisible (sendHeader);

    addAndMakeVisible (hostLabel);
    hostEditor.setJustification (juce::Justification::centredLeft);
    hostEditor.onReturnKey  = [this] { applyHostEdit(); };
    hostEditor.onFocusLost  = [this] { applyHostEdit(); };
    addAndMakeVisible (hostEditor);

    addAndMakeVisible (portLabel);
    portEditor.setInputRestrictions (5, "0123456789");
    portEditor.setJustification (juce::Justification::centredLeft);
    portEditor.onReturnKey  = [this] { applyPortEdit(); };
    portEditor.onFocusLost  = [this] { applyPortEdit(); };
    addAndMakeVisible (portEditor);

    enableToggle.onClick = [this] { settings.setEnabled (enableToggle.getToggleState()); };
    addAndMakeVisible (enableToggle);

    syncHintLabel.setText (juce::String ("Clients ziehen den Ist-Zustand per ")
                               + osc::syncAddress + " (Voll-Dump).",
                           juce::dontSendNotification);
    syncHintLabel.setColour (juce::Label::textColourId,
                             juce::Colours::white.withAlpha (0.55f));
    syncHintLabel.setFont (juce::Font (juce::FontOptions (12.0f)));
    addAndMakeVisible (syncHintLabel);

    settings.addChangeListener (this);
    syncControls();
}

OscSettingsComponent::~OscSettingsComponent()
{
    settings.removeChangeListener (this);
}

//==============================================================================
void OscSettingsComponent::applyHostEdit()
{
    settings.setHost (hostEditor.getText());
    syncControls();  // verworfene Eingaben (leer) zurückdrehen
}

void OscSettingsComponent::applyPortEdit()
{
    settings.setPort (portEditor.getText().getIntValue());
    syncControls();
}

//==============================================================================
void OscSettingsComponent::changeListenerCallback (juce::ChangeBroadcaster*)
{
    syncControls();
}

void OscSettingsComponent::syncControls()
{
    hostEditor.setText (settings.getHost(), juce::dontSendNotification);
    portEditor.setText (juce::String (settings.getPort()), juce::dontSendNotification);
    enableToggle.setToggleState (settings.isEnabled(), juce::dontSendNotification);
    updateStatusLabels();
}

void OscSettingsComponent::updateStatusLabels()
{
    const auto port = oscController.getConnectedPort();

    // Umlaute als escaped UTF-8 — MSVC liest BOM-lose Quellen als CP1252
    receiveStatusLabel.setText (port > 0
                                    ? juce::String::fromUTF8 ("Empf\xc3\xa4ngt auf UDP-Port ")
                                          + juce::String (port)
                                    : juce::String ("Nicht verbunden"),
                                juce::dontSendNotification);
}

//==============================================================================
void OscSettingsComponent::layoutRow (juce::Rectangle<int>& area, juce::Label& label,
                                      juce::Component& control, int rowHeight)
{
    auto row = area.removeFromTop (rowHeight);
    label.setBounds (row.removeFromLeft (110));
    control.setBounds (row.reduced (0, 2));
    area.removeFromTop (6);
}

void OscSettingsComponent::resized()
{
    auto area = getLocalBounds().reduced (18);

    receiveHeader.setBounds (area.removeFromTop (28));
    receiveStatusLabel.setBounds (area.removeFromTop (24));
    area.removeFromTop (14);

    sendHeader.setBounds (area.removeFromTop (28));
    area.removeFromTop (8);

    layoutRow (area, hostLabel, hostEditor);
    layoutRow (area, portLabel, portEditor);

    enableToggle.setBounds (area.removeFromTop (30));
    area.removeFromTop (10);
    syncHintLabel.setBounds (area.removeFromTop (20));
}

} // namespace conduit
