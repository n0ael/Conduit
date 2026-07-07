#pragma once

#include <vector>

#include <juce_core/juce_core.h>

#include "Core/GridVoiceEngine.h"

namespace conduit
{

//==============================================================================
/**
    Reine Fade-Zustands-Logik der Live-Noten-Kreise im MPE-Shaping-Editor
    (MpeShapingView, S2c) -- headless, testbar, kein Rendering.

    update() bekommt pro Frame die aktuell aktiven Stimmen EINER Achse
    (GridVoiceEngine::readActiveVoices) und die seit dem letzten Aufruf
    vergangene Zeit in ms: aktive Stimmen erscheinen/bleiben mit Deckkraft 1
    und ihrem aktuellen Rohwert; Stimmen, die nicht mehr aktiv sind, fadet
    linear über fadeMs auf 0 (der zuletzt bekannte Rohwert bleibt dabei
    eingefroren, damit der Kreis an seiner letzten Position ausfadet) und
    wird danach entfernt. Wiederauftauchen setzt die Deckkraft sofort auf 1
    zurück ("erneut angeschlagen").

    Message Thread (MpeShapingView-VBlank-Tick).
*/
class NoteCircleFadeTracker
{
public:
    struct Circle
    {
        int   voiceIndex   = -1;
        float rawValue     = 0.0f;   // eingefroren, sobald die Stimme inaktiv wird
        float opacity      = 1.0f;
        bool  stillActive  = true;  // internes Bookkeeping (nächstes update())
    };

    explicit NoteCircleFadeTracker (int fadeMsToUse = 180) noexcept : fadeMs (fadeMsToUse) {}

    void setFadeMs (int newFadeMs) noexcept { fadeMs = juce::jmax (0, newFadeMs); }
    [[nodiscard]] int getFadeMs() const noexcept { return fadeMs; }

    /** Ein Frame: active = aktuelle readActiveVoices()-Liste dieser Achse,
        deltaMs = seit dem vorigen update() vergangene Zeit. */
    void update (const std::vector<grid::GridVoiceEngine::VoiceReadout>& active, double deltaMs);

    /** Alle aktuell zu zeichnenden Kreise (aktiv ODER noch ausfadend). */
    [[nodiscard]] const std::vector<Circle>& circles() const noexcept { return tracked; }

private:
    std::vector<Circle> tracked;
    int fadeMs;
};

} // namespace conduit
