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

    // Learn-Probe (7.3): OSCReceiver liefert keine Absender-IP — der
    // Controller lauscht kurz selbst auf dem Empfangsport
    learnButton.onClick = [this]
    {
        if (oscController.isLearning())
        {
            oscController.cancelIpLearn();
            learnStatusLabel.setText ("Abgebrochen.", juce::dontSendNotification);
            updateLearnUi();
            return;
        }

        // Das Fenster kann während der Probe geschlossen werden — die
        // Settings überleben (EngineProcessor), die UI nur via SafePointer
        juce::Component::SafePointer<OscSettingsComponent> safeThis (this);
        auto* settingsPtr = &settings;

        const auto started = oscController.beginIpLearn (
            [safeThis, settingsPtr] (const juce::String& ip)
            {
                if (ip.isNotEmpty())
                    settingsPtr->setHost (ip);

                if (safeThis == nullptr)
                    return;

                safeThis->learnStatusLabel.setText (
                    ip.isNotEmpty() ? "Gelernt: " + ip
                                    : juce::String ("Kein Paket empfangen (Timeout)."),
                    juce::dontSendNotification);
                safeThis->syncControls();
                safeThis->updateLearnUi();
            });

        learnStatusLabel.setText (started
                                      ? juce::String ("Warte auf ein UDP-Paket vom Sender ...")
                                      : juce::String ("Empfang nicht verbunden."),
                                  juce::dontSendNotification);
        updateLearnUi();
    };
    addAndMakeVisible (learnButton);

    learnStatusLabel.setColour (juce::Label::textColourId,
                                juce::Colours::white.withAlpha (0.7f));
    addAndMakeVisible (learnStatusLabel);

    syncHintLabel.setText (juce::String ("Clients ziehen den Ist-Zustand per ")
                               + osc::syncAddress + " (Voll-Dump).",
                           juce::dontSendNotification);
    syncHintLabel.setColour (juce::Label::textColourId,
                             juce::Colours::white.withAlpha (0.55f));
    syncHintLabel.setFont (juce::Font (juce::FontOptions (12.0f)));
    addAndMakeVisible (syncHintLabel);

    settings.addChangeListener (this);
    syncControls();
    updateLearnUi();
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

void OscSettingsComponent::updateLearnUi()
{
    learnButton.setButtonText (oscController.isLearning()
                                   ? "Lernen abbrechen"
                                   : "IP vom letzten Sender lernen");
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

    learnButton.setBounds (area.removeFromTop (30).withWidth (240));
    area.removeFromTop (4);
    learnStatusLabel.setBounds (area.removeFromTop (24));
    area.removeFromTop (10);

    syncHintLabel.setBounds (area.removeFromTop (20));
}

} // namespace conduit
