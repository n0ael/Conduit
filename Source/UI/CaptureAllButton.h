#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "Core/Capture/CaptureService.h"

namespace conduit
{

//==============================================================================
/**
    "Capture All"-Button der Toolbar (~60 px breit, Touch-Target ≥ 44 px,
    CLAUDE.md 10) — neben dem Link-Transport, bis die Kanal-Buttons mit dem
    Mixer-Meilenstein in die Channel-Strips wandern.

    Read-only gegenüber der Engine: Der Ring um den Button zeigt den
    aggregierten Status (idle/recording/held) plus eine dezente
    Füllstandsanzeige, ein innerer Bogen den laufenden Export. Gefüttert
    wird er ausschließlich von außen über setStatus() — der
    EngineEditor-Timer (15 Hz) pollt die Status-Atomics des CaptureService;
    der Button selbst hat bewusst KEINEN eigenen Timer. Repaint nur bei
    sichtbarer Änderung (Füllstand quantisiert).

    juce::Button liefert onClick, Maus/Keyboard-Parität und Accessibility.
*/
class CaptureAllButton : public juce::Button
{
public:
    CaptureAllButton();

    /** [Message Thread, Editor-Timer] Repaint nur bei sichtbarer Änderung. */
    void setStatus (const CaptureService::UiStatus& newStatus);

private:
    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted,
                      bool shouldDrawButtonAsDown) override;

    /** Füllstand auf 48 Stufen quantisiert — Repaint-Schwelle. */
    [[nodiscard]] static int quantizeFill (float fillNorm) noexcept;

    CaptureService::UiStatus status;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CaptureAllButton)
};

} // namespace conduit
