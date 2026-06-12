#pragma once

#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "Core/Capture/CaptureService.h"
#include "Core/Capture/CaptureSettings.h"

namespace conduit
{

//==============================================================================
/**
    Einklappbares Capture-Panel unter der Toolbar — Übergangslösung, bis die
    Kanal-Anzeigen mit dem Mixer-Meilenstein in die Channel-Strips wandern.

    Read/listen-only gegenüber der Engine (CLAUDE.md 6): die Controls
    schreiben ausschließlich in CaptureSettings [Message Thread], die
    Kanal-Leiste rechts ist reine Anzeige und wird vom EngineEditor-Timer
    über refresh() gefüttert (Status-Atomics des CaptureService, kein
    eigener Timer, Repaint nur bei Änderung).

    Resize-Policy-UI (CaptureSettings-Doku): bufferMinutes/preRollSeconds
    laufen über die Settings-Setter; bei aktiver Aufnahme feuert
    onPendingResize → async Ok/Cancel-AlertWindow
    (JUCE_MODAL_LOOPS_PERMITTED=0) → confirm-/cancelPendingResize.

    "Nach Export freigeben" schaltet nur die Nachfrage frei — der
    RAM-Puffer wird NIE ohne Bestätigung geleert (der Dialog selbst kommt
    vom EngineEditor beim Export-Abschluss).

    Touch-first (10): alle Controls 44 px hoch, Maus/Keyboard-Parität über
    native JUCE-Controls.
*/
class CapturePanel : public juce::Component,
                     private juce::ChangeListener
{
public:
    static constexpr int preferredHeight = 128;

    CapturePanel (CaptureSettings& settingsToUse, CaptureService& serviceToUse);
    ~CapturePanel() override;

    /** [Message Thread, Editor-Timer] Kanal-Leiste aus den Status-Atomics
        aktualisieren — Repaint nur bei sichtbarer Änderung. */
    void refresh();

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;
    void syncControls();
    void chooseExportDirectory();
    void applyRingSlider (juce::Slider& slider, bool isBufferMinutes);

    CaptureSettings& settings;
    CaptureService& service;

    juce::Slider thresholdSlider { juce::Slider::LinearBar, juce::Slider::TextBoxLeft };
    juce::Slider holdSlider      { juce::Slider::LinearBar, juce::Slider::TextBoxLeft };
    juce::Slider preRollSlider   { juce::Slider::LinearBar, juce::Slider::TextBoxLeft };
    juce::Slider bufferSlider    { juce::Slider::LinearBar, juce::Slider::TextBoxLeft };
    // Umlaute als escaped UTF-8 — MSVC liest BOM-lose Quellen als CP1252
    juce::ToggleButton autoCalibrateToggle      { "Auto-Schwelle" };
    juce::ToggleButton releaseAfterExportToggle {
        juce::String::fromUTF8 ("Nach Export freigeben (mit R\xc3\xbc" "ckfrage)") };
    juce::ComboBox bitDepthCombo;
    juce::TextButton directoryButton { "Ordner..." };
    juce::Label directoryLabel;
    juce::Label ramWarningLabel;

    struct ChannelSnapshot
    {
        CaptureChannel::State state = CaptureChannel::State::idle;
        int fillSteps = 0;  // quantisiert, Repaint-Schwelle
    };
    std::vector<ChannelSnapshot> channelSnapshots;
    juce::Rectangle<int> channelArea;

    // Muss den async Callback überleben (JUCE_MODAL_LOOPS_PERMITTED=0)
    std::unique_ptr<juce::FileChooser> directoryChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CapturePanel)
};

} // namespace conduit
