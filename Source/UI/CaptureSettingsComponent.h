#pragma once

#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "Core/Capture/CaptureService.h"
#include "Core/Capture/CaptureSettings.h"

namespace conduit
{

//==============================================================================
/**
    Capture-Einstellungen als Formular — der „Capture"-Tab des
    Einstellungen-Fensters (CLAUDE.md 10/13.2). Schreibt ausschließlich in
    CaptureSettings [Message Thread]; die eigentlichen Capture-Aktionen
    (Einzel-/Alles-Aufnahme) bleiben in der Toolbar/CapturePanel.

    Enthält: Schwelle (Threshold), Hold, Pre-Roll, Ring-Puffer, Auto-Schwelle,
    RAM-Limit, Bit-Tiefe, Export-Ordner, „nach Export freigeben". Ring-Puffer
    und Pre-Roll folgen der Resize-Policy (bei aktiver Aufnahme async
    Bestätigung, CaptureSettings-Doku). RAM-Warnung wird über den
    CaptureService-ChangeBroadcast eingeblendet.
*/
class CaptureSettingsComponent final : public juce::Component,
                                       private juce::ChangeListener
{
public:
    CaptureSettingsComponent (CaptureSettings& settingsToUse, CaptureService& serviceToUse);
    ~CaptureSettingsComponent() override;

    void resized() override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;
    void syncControls();
    void applyRingSlider (juce::Slider& slider, bool isBufferMinutes);
    void chooseExportDirectory();

    /** Beschriftete Zeile (Label links, Control rechts) im Formular-Layout. */
    void layoutRow (juce::Rectangle<int>& area, juce::Label& label,
                    juce::Component& control, int rowHeight = 30);

    CaptureSettings& settings;
    CaptureService& service;

    juce::Label thresholdLabel { {}, "Schwelle" };
    juce::Label holdLabel      { {}, "Hold" };
    juce::Label preRollLabel   { {}, "Pre-Roll" };
    juce::Label bufferLabel    { {}, "Ring-Puffer" };
    juce::Label ramLimitLabel  { {}, "RAM-Limit" };
    juce::Label bitDepthLabel  { {}, "Bit-Tiefe" };
    juce::Label directoryHeader;

    juce::Slider thresholdSlider { juce::Slider::LinearBar, juce::Slider::TextBoxLeft };
    juce::Slider holdSlider      { juce::Slider::LinearBar, juce::Slider::TextBoxLeft };
    juce::Slider preRollSlider   { juce::Slider::LinearBar, juce::Slider::TextBoxLeft };
    juce::Slider bufferSlider    { juce::Slider::LinearBar, juce::Slider::TextBoxLeft };
    juce::Slider ramLimitSlider  { juce::Slider::LinearBar, juce::Slider::TextBoxLeft };
    juce::ComboBox bitDepthCombo;

    // Umlaute als escaped UTF-8 — MSVC liest BOM-lose Quellen als CP1252
    juce::ToggleButton autoCalibrateToggle      { "Auto-Schwelle" };
    juce::ToggleButton releaseAfterExportToggle {
        juce::String::fromUTF8 ("Nach Export freigeben (mit R\xc3\xbc" "ckfrage)") };

    juce::TextButton directoryButton { juce::String::fromUTF8 ("Ordner \xe2\x80\xa6") };
    juce::Label directoryLabel;
    juce::Label ramWarningLabel;

    // Muss den async Callback überleben (JUCE_MODAL_LOOPS_PERMITTED=0)
    std::unique_ptr<juce::FileChooser> directoryChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CaptureSettingsComponent)
};

} // namespace conduit
