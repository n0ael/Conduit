#pragma once

#include <map>

#include <juce_core/juce_core.h>

namespace conduit::grid
{

//==============================================================================
/** Feder-Parameter des Grid-Physics-Kerns (Block J, Masterplan): EIN
    gemeinsamer Parametersatz für Grid-Gravity (J1) und Fader/XY-Physics
    (J3) — force ist die Federkonstante k, mass die träge Masse, inertia01
    steuert die Dämpfung (0 = kriecht ohne Schwingen ans Ziel, 1 = federt
    lange nach — Überschwingen um den Zielwert). */
struct SpringParams
{
    float force     = 400.0f;   // Federkonstante k (Einheiten: Ziel-Einheit/s² bei mass 1)
    float mass      = 1.0f;
    float inertia01 = 0.4f;     // 0..1 → Dämpfungsgrad ζ 1.1..0.12 (unter 1 = Überschwingen)
};

/** Zustand einer 1-D-Feder (Position + Geschwindigkeit). */
struct SpringState
{
    float pos = 0.0f;
    float vel = 0.0f;
};

/** Ein Zeitschritt der gedämpften Feder Richtung target — semi-implizites
    Euler, intern in ≤ 5-ms-Teilschritten integriert (stabil auch bei
    ausgelassenen Frames). forceScale01 skaliert NUR die Federkraft (die
    Dämpfung bleibt voll — bei 0 klingt die Bewegung aus, driftet nicht). */
void stepSpring (SpringState& state, float target, const SpringParams& params,
                 float dtSeconds, float forceScale01 = 1.0f) noexcept;

//==============================================================================
/**
    Grid-Gravity (Block J1, Masterplan): pro liegendem Finger (Sonne) zieht
    ein Pad-Magnet den EFFEKTIVEN Pitch-Rohwert (X vor der Treppen-Kennlinie,
    CLAUDE.md-Spec „schreibt in den Pitch-Rohwert VOR der Kurve") auf den
    nächstgelegenen perfekten Pitch — mit Masse-/Feder-Verhalten inklusive
    Überschwingen (SpringParams).

    Koordinaten in PAD-BREITEN (1.0 = eine Spalte = perfekter Halbtonschritt
    relativ zum In-Tune-Anker); der Aufrufer (GridKeyboardComponent)
    konvertiert normX ↔ Pads über die Spaltenzahl.

    Ablauf pro Finger:
      - Bewegung über movementThresholdPadsPerSec ⇒ der Magnet lässt los
        (Kraft blendet schnell aus), der Finger hat die direkte Kontrolle.
      - Stillstand länger als delayMs ⇒ die Magnetkraft blendet über fadeMs
        ein (fade-time-force) und federt den effektiven X-Wert auf
        anchor + round(touch − anchor) — die Feder schwingt hörbar über den
        perfekten Pitch hinaus und pendelt sich ein.
      - Solange der Magnet nie gegriffen hat, ist effectiveX EXAKT der
        Touch-Wert (verlustfreier Bypass — identisches Verhalten wie ohne
        Gravity).

    Message Thread (der Besitzer tickt über einen VBlank-/Timer-Puls,
    Masterplan „Message-Thread-Timer"). Headless testbar.
*/
class GridGravity
{
public:
    struct Config
    {
        SpringParams spring;
        int   delayMs = 150;                       // Stillstand, bis der Magnet greift
        float movementThresholdPadsPerSec = 0.5f;  // darüber = Finger bewegt sich aktiv
        int   fadeMs = 250;                        // Kraft-Einblendzeit (fade-time-force)
    };

    void setConfig (const Config& newConfig) noexcept { config = newConfig; }

    /** Finger aufgesetzt: xPads = aktuelle X-Position, anchorPads = In-Tune-
        Anker (Pad-Zentrum bzw. Aufsetzpunkt, Block B1) — beide in Pad-Breiten. */
    void onDown (int fingerId, float xPads, float anchorPads);

    /** Fingerbewegung — akkumuliert die Strecke für die Threshold-Messung. */
    void onMove (int fingerId, float xPads) noexcept;

    void onUp (int fingerId) noexcept;
    void clear() noexcept;

    /** Ein Simulationsschritt für alle liegenden Finger; true, wenn sich
        mindestens ein effektiver Wert hörbar bewegt hat (Sende-/Repaint-
        Trigger für den Besitzer). */
    bool tick (float dtSeconds);

    /** Effektiver X-Wert (Pads) — exakt fallbackXPads, solange der Magnet
        für diesen Finger nie eingegriffen hat oder die Id unbekannt ist. */
    [[nodiscard]] float effectiveX (int fingerId, float fallbackXPads) const noexcept;

    /** Magnetkraft-Anteil 0..1 dieses Fingers (Schatten-/Debug-Optik). */
    [[nodiscard]] float pullAmount (int fingerId) const noexcept;

    [[nodiscard]] bool hasFingers() const noexcept { return ! fingers.empty(); }

private:
    struct Finger
    {
        float touchX  = 0.0f;
        float anchorX = 0.0f;
        SpringState eff;             // gültig, sobald engaged
        bool  engaged      = false;  // Magnet hat (mindestens einmal) gegriffen
        float stillSeconds = 0.0f;
        float movedAccum   = 0.0f;   // Strecke seit dem letzten tick (Pads)
        float fade         = 0.0f;   // Kraft-Einblendung 0..1
    };

    std::map<int, Finger> fingers;
    Config config;
};

} // namespace conduit::grid
