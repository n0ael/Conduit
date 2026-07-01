#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "Core/MeterSettings.h"

namespace conduit
{

//==============================================================================
/**
    Gebündeltes Einstellungen-Fenster (CLAUDE.md 10) — ein Einstiegspunkt für
    die App-Einstellungen, als Tabs:
      - „Audio-Gerät" (juce::AudioDeviceSelectorComponent via
        AudioSettingsComponent) — nur wenn ein AudioDeviceManager vorliegt
        (Standalone-Pfad).
      - „Metering" (Clip-Reset-Modus, bindet MeterSettings).

    Capture-Einstellungen folgen als eigener Tab (Schritt 3b). Wird non-modal
    in einem juce::DialogWindow gezeigt (EngineEditor, launchAsync). Dark-Look
    via LookAndFeel_V4 (Midnight), analog AudioSettingsComponent.
*/
class SettingsWindow final : public juce::Component
{
public:
    SettingsWindow (juce::AudioDeviceManager* deviceManager, MeterSettings& meterSettings);
    ~SettingsWindow() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::LookAndFeel_V4 darkLook { juce::LookAndFeel_V4::getMidnightColourScheme() };
    juce::TabbedComponent tabs { juce::TabbedButtonBar::Orientation::TabsAtTop };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SettingsWindow)
};

} // namespace conduit
