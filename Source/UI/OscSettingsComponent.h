#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "Core/OscController.h"
#include "Core/OscSendSettings.h"

namespace conduit
{

//==============================================================================
/**
    OSC-Einstellungen als Formular — der „OSC"-Tab des Einstellungen-Fensters
    (CLAUDE.md 7.3/10). Empfangs-Status (Port aus OscController) plus das
    Send-Ziel: Host, Port, Enable — schreibt ausschließlich in OscSendSettings
    [Message Thread]; der OscSendService folgt deren ChangeBroadcast.

    Controls public — die headless UI-Tests treiben sie direkt
    (Projektkonvention, siehe LinkAudioSendPanel).
*/
class OscSettingsComponent final : public juce::Component,
                                   private juce::ChangeListener
{
public:
    OscSettingsComponent (OscSendSettings& settingsToUse, OscController& oscControllerToUse);
    ~OscSettingsComponent() override;

    void resized() override;

    //==========================================================================
    // Controls public für headless Tests
    juce::TextEditor hostEditor;
    juce::TextEditor portEditor;
    juce::ToggleButton enableToggle { "Parameter-Feedback senden" };

    /** Übernimmt Host/Port aus den Editoren in die Settings (Return/Focus-
        Lost der Editoren — Tests rufen direkt). */
    void applyHostEdit();
    void applyPortEdit();

private:
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;

    /** Settings → Controls (Start + jeder Settings-Broadcast). */
    void syncControls();
    void updateStatusLabels();

    /** Beschriftete Zeile (Label links, Control rechts) im Formular-Layout. */
    void layoutRow (juce::Rectangle<int>& area, juce::Label& label,
                    juce::Component& control, int rowHeight = 30);

    OscSendSettings& settings;
    OscController& oscController;

    juce::Label receiveHeader;
    juce::Label receiveStatusLabel;
    juce::Label sendHeader;
    juce::Label hostLabel { {}, "Ziel-Host" };
    juce::Label portLabel { {}, "Ziel-Port" };
    juce::Label syncHintLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OscSettingsComponent)
};

} // namespace conduit
