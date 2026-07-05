#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "Core/LooperSettings.h"
#include "PushLookAndFeel.h"

namespace conduit
{

//==============================================================================
/**
    Einstellungs-Menü der Looper-Page (M6, CallOutBox-Inhalt — Muster
    TapMenuPanel): alle Verhaltens-Optionen, die der User bewusst als
    Menüpunkte wollte (Entscheidungen 05.07.2026). Bindet DIREKT an die
    LooperSettings (App-Zustand, kein Engine-Zugriff) — der
    EngineProcessor folgt über seinen ChangeListener, die Page über den
    Editor-Refresh.

    Punkte: Launch-Quantisierung (Start/Stop; Commit bleibt sofort) ·
    Tap auf spielenden Clip (Retrigger/Stop) · ÷2-Hälfte · Reverse-Punkt ·
    VARI-Raster (Halbtöne/Session-Skala) · VARI-Scope (Track/Looper) ·
    Solo-Scope · Sichtbare Slots (4–12) · Delete als Schalter (Latch,
    Nicht-Touch) · Auto-Advance.
*/
class LooperSettingsMenu final : public juce::Component
{
public:
    explicit LooperSettingsMenu (LooperSettings& settingsToUse);

    void resized() override;
    void paint (juce::Graphics& g) override;

    // Test-Zugriff
    [[nodiscard]] juce::ComboBox& getQuantCombo() noexcept { return quantCombo; }
    [[nodiscard]] juce::ComboBox& getTapModeCombo() noexcept { return tapModeCombo; }
    [[nodiscard]] juce::ComboBox& getSlotsCombo() noexcept { return slotsCombo; }
    [[nodiscard]] juce::ToggleButton& getDeleteLatchToggle() noexcept { return deleteLatchToggle; }
    [[nodiscard]] juce::ToggleButton& getAutoAdvanceToggle() noexcept { return autoAdvanceToggle; }

private:
    struct Row
    {
        juce::Label caption;
        juce::Component* control = nullptr;
    };

    void addRow (const juce::String& caption, juce::Component& control);

    LooperSettings& settings;

    juce::ComboBox quantCombo, tapModeCombo, halveCombo, reverseCombo,
                   variRasterCombo, variScopeCombo, soloScopeCombo, slotsCombo;
    juce::ToggleButton deleteLatchToggle, autoAdvanceToggle;

    std::vector<std::unique_ptr<Row>> rows;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LooperSettingsMenu)
};

} // namespace conduit
