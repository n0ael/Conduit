#pragma once

#include <array>
#include <map>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "Core/ChordMemory.h"
#include "Core/GridPanelSettings.h"
#include "Core/GridPhysics.h"
#include "Core/GridVoiceEngine.h"
#include "Core/PadGridLayout.h"
#include "Core/RingTouchModel.h"
#include "Util/ScaleQuantizer.h"

namespace conduit
{

//==============================================================================
/**
    Touch-Fläche des Grid-Voice-Modells (M1 Teil 3, CLAUDE.md 14 ADR
    Grid-Page): isomorphes Pad-Raster (PadGridLayout), Multi-Touch über die
    Standard-Maus-/Touch-Callbacks — jeder JUCE-Touch-Source liefert ein
    eigenes event.source.

    Note + Pitch-Bend über X + Pressure über Y (M1 Teil 3) plus Ring-
    Mechanik als zweite Ausdrucksachse (M1b-2, "Sonne/Mond/Orbit"): ein
    zweiter Finger im Greifband eines liegenden primären Fingers (Sonne)
    wird dessen Ring-Finger (Mond), sein Abstand zur Sonne (Orbit-Radius)
    moduliert setSlide des primären Fingers (RingTouchModel).
    Bewusst weiterhin ohne Drone/Latch, Pinch, Doppeltipp, Drift-über-Rand,
    Rand-Ribbons, Release-All — eigene Meilensteine.

    Hält keinen eigenen Zustand außer der Per-Finger-Zuordnung (primär),
    der Session-Skala (setScale) und dem RingTouchModel; ruft die
    GridVoiceEngine& direkt (Message Thread, CLAUDE.md 4.2 ITouchMacro).
*/
class GridKeyboardComponent final : public juce::Component
{
public:
    explicit GridKeyboardComponent (grid::GridVoiceEngine& engineToUse,
                                     const grid::PadGridLayout::Config& layoutConfig = {});

    void paint (juce::Graphics& g) override;

    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseUp   (const juce::MouseEvent& event) override;

    //==========================================================================
    // Testbare Kernpfade der Maus-/Touch-Handler (Block M — Muster
    // HoldIconTile/TrackTabsStrip: die Maus-Handler delegieren NUR, damit
    // Multi-Finger-Abläufe headless mit expliziten fingerIds prüfbar sind).

    void touchDown (int fingerId, juce::Point<float> position);
    void touchMove (int fingerId, juce::Point<float> position);
    void touchUp   (int fingerId, juce::Point<float> position);

    /** Session-Skala (Root-Tree scaleRoot/scaleType, Schema 6.2) — färbt die
        Pad-Grundfarben (Grundton/Skalenton/skalenfremd). Message Thread. */
    void setScale (int newRootNote, ScaleType newScaleType);

    /** Pad-Grundfarbe nach Session-Skala (Design-Mock Grid-Page v2):
        Grundton padRoot, Skalenton tile, skalenfremd padUnlit — pure
        function, testbar ohne Component. */
    [[nodiscard]] static juce::Colour padBaseColour (int midiNote, int rootNote,
                                                     ScaleType type) noexcept;

    //==========================================================================
    // Akkord-Speicher (Grid-Page v2, Feature 6): eine abgerufene
    // Konstellation liegt "latched" auf dem Grid — ohne physische Finger —
    // und wird wie Sonnen/Monde gerendert (inkl. Pad-Glow). Message Thread.

    /** Ruft eine gespeicherte Konstellation ab: beendet zuerst den evtl.
        liegenden Akkord (clearLatched), dann pro Sonne noteOn +
        Startwerte (Bend 0, Pressure neutral, Slide aus dem Mond-Offset).
        Sonnen außerhalb des Rasters werden nur visuell gelatched. */
    void latchConstellation (const std::vector<grid::StoredSun>& suns);

    /** Verschiebt die latched Konstellation starr um das Pixel-Delta —
        KEIN Clamping (Sonnen dürfen über den Rand, nur visuell geclippt).
        X-Bewegung = Pitch-Bend, Y = Ausdruck — exakt wie ein Finger-Drag;
        der Mond-Offset (Slide) bleibt starr. */
    void moveLatchedBy (float dxPx, float dyPx);

    /** Beendet den latched Akkord (noteOff, releaseVelocity 0) und
        entfernt die Konstellation. */
    void clearLatched();

    /** Aktuelle Konstellation = live Sonnen (RingTouchModel) PLUS latched —
        normalisiert (x über Breite, y über Höhe, ox/oy BEIDE über die
        Breite, ChordMemory-Konvention). */
    [[nodiscard]] std::vector<grid::StoredSun> constellationNormalized() const;

    //==========================================================================
    // Sensitivity-/Range-Regler (Block A2/A3, MpeShapingView-Detailspalte):
    // skalieren IMMER von den beim Ctor gecachten Basiswerten aus, nie vom
    // aktuellen Config-Wert -- verhindert Drift bei wiederholtem Setzen.
    // Laufzeit-only, keine Persistenz (kommt gebündelt in Block K).

    /** Pressure-Sensitivity 0..100 (50 = heutiges Verhalten, hoeher = mehr
        Fingerweg = feiner). Skaliert PadGridLayout::yRangeNorm. */
    void setPressureSensitivity (double sensitivity0to100) noexcept;
    /** Slide-Sensitivity 0..100, analog ueber die Ring-Radiusspanne. */
    void setSlideSensitivity (double sensitivity0to100) noexcept;
    /** PitchBend-Range-Multiplikator (Block A3: 0.25/0.5/1/2/4/8). Skaliert
        PadGridLayout::semitonesPerPadWidth; der 48-HT-Ausgangs-Clamp der
        GridVoiceEngine bleibt unangetastet. */
    void setPitchBendMultiplier (float multiplier) noexcept;

    /** In-Tune-Anker des Pitch-Bends (Block B1): pad (Default, Push-
        Paradigma -- Pad-Zentrum in tune, Finger bendet absolut, Re-Hit
        derselben Position = identischer Pitch) oder finger (Aufsetzpunkt =
        0 Bend, bisheriges Verhalten). Wirkt ab dem naechsten Anschlag. */
    void setInTuneLocation (grid::InTuneLocation newLocation) noexcept { inTuneLocation = newLocation; }
    [[nodiscard]] grid::InTuneLocation getInTuneLocation() const noexcept { return inTuneLocation; }

    /** In-Tune-Width 0..100 % der Pad-Breite (Block B2/D1). */
    void setInTuneWidthPercent (float newPercent) noexcept { layout.setInTuneWidthPercent (newPercent); }

    //==========================================================================
    // Oktav-Shift (Block D2, Octave-Up/Down-Buttons ueber dem Pitch-Fader):
    // verschiebt lowestNote um Vielfache von 12 relativ zur Ctor-Basis.
    // Laufzeit-only (Block K persistiert), geklemmt auf +-3 Oktaven, damit
    // das Raster nicht aus dem MIDI-Bereich [0,127] laeuft.

    void octaveUp() noexcept;
    void octaveDown() noexcept;
    [[nodiscard]] int octaveShift() const noexcept { return octaveShiftValue; }

    static constexpr int kMaxOctaveShift = 3;

    //==========================================================================
    // Noten-Echo (Block H4, User-Wunsch 11.07.2026): extern gespielte Noten
    // (Lives Wiedergabe über den „Conduit DAW"-Rückweg) leuchten in der
    // Fokus-Track-Farbe auf ALLEN isomorphen Pad-Positionen der Note —
    // OHNE Sonne/Mond (reines Feedback, keine Interaktion). Message Thread.

    void setEchoColour (juce::Colour newColour) noexcept;
    void echoNoteOn (int midiNote, float velocity01) noexcept;
    void echoNoteOff (int midiNote) noexcept;
    void clearEchoNotes() noexcept;

    /** Echo-Stärke einer Note [0,1] (0 = aus) — testbarer Zustand. */
    [[nodiscard]] float echoLevel (int midiNote) const noexcept;

    /** Block I: Farbe der Root-Pads — Default padRoot-Grau; die GridPage
        setzt hier die Fokus-Track-Farbe, wenn das Settings-Toggle
        „Root in Track-Farbe" an ist (wie Push). */
    void setRootPadColour (juce::Colour newColour) noexcept;

    //==========================================================================
    // Grid-Gravity (Block J1) + Pitch-Schatten (J2): Pad-Magnet zieht den
    // Pitch-Rohwert liegender Finger VOR der Treppen-Kennlinie auf den
    // nächstgelegenen perfekten Pitch (grid::GridGravity, Feder mit
    // Überschwingen); der Schatten (Abdunkelung + weiche Kante) markiert
    // die X-Position des tatsächlich KLINGENDEN Pitch. Konfiguration wird
    // pro VBlank-Frame live aus den GridPanelSettings gepollt (Muster
    // TrackTabsStrip — Toggle im Settings-Tab, Tuning im Dev-Panel).

    /** nullptr (Default/Tests) = Gravity aus, keine Schatten. */
    void setPanelSettings (const GridPanelSettings* settingsToPoll) noexcept
    {
        panelSettings = settingsToPoll;
    }

    //==========================================================================
    // Hold/Drone (Block M, Masterplan — User-Semantik 12.07.2026):
    // Abhebe-Reihenfolge „Sonne zuerst": hebt der Sonnen-Finger ab, während
    // sein Mond noch LIEGT, dront die Note weiter — die Sonne bleibt
    // fingerlos auf dem Grid stehen, der Orbit friert ein (der noch
    // liegende Mond-Finger wird ignoriert, bis er abhebt). Kurzer Tap auf
    // die Drone-Sonne beendet sie (noteOff); Aufsetzen + Halten/Bewegen
    // ÜBERNIMMT die Stimme nahtlos (kein Neuanschlag, Bend/Pressure wirken
    // RELATIV ab dem Aufsetzpunkt — nie zur Fingerposition springen);
    // Loslassen nach einem Grab legt den Drone wieder ab. Release-All
    // beendet auch alle Drones (clearDrones). Message Thread.

    /** Beendet alle Drones (noteOff) — Release-All ruft dies zusätzlich zu
        engine.allNotesOff (idempotent). */
    void clearDrones();

    [[nodiscard]] int droneCount() const noexcept { return (int) drones.size(); }

private:
    struct FingerState
    {
        float startNormX  = 0.0f;
        float startNormY  = 0.0f;
        float anchorNormX = 0.0f;   // In-Tune-Anker (Block B1): Pad-Zentrum
                                    // (pad-Modus) oder Aufsetzpunkt (finger)
        float currentNormX = 0.0f;  // letzte Touch-Position (Block J: Gravity-
        float currentNormY = 0.0f;  // Tick + Pitch-Schatten)

        // Block M: zuletzt an die Engine gesendete Werte — beim Drone-Start
        // friert die Stimme exakt hier ein.
        float lastBendSemitones = 0.0f;
        float lastPressure01    = 0.5f;

        // Block M: Drone-Grab (Finger hat eine Drone-Sonne aufgenommen) —
        // 0 = normaler Spiel-Finger. Bend/Pressure wirken RELATIV ab dem
        // Aufsetzpunkt (grab*-Referenzen), Tap-Erkennung über Weg + Zeit.
        uint32_t grabbedDroneId = 0;
        float grabNormX = 0.0f, grabNormY = 0.0f;
        float grabBendSemitones = 0.0f;
        float grabPressure01    = 0.5f;
        juce::Point<float> downPx;
        float  maxMovePx  = 0.0f;
        double downTimeMs = 0.0;
    };

    /** Fingerlos liegende Drone-Sonne (Block M): die Stimme läuft unter
        einer synthetischen fingerId weiter (kDroneFingerBase+n — Touch-Ids
        werden vom OS wiederverwendet, rekeyVoice beim Drone-Start). */
    struct DroneSun
    {
        juce::Point<float> centre;        // px, folgt einem Grab
        juce::Point<float> orbitOffset;   // px, eingefroren (starr, wie Latch)
        bool hasOrbit = false;
        uint32_t voiceFingerId = 0;
        float anchorNormX = 0.0f;         // In-Tune-Anker der Original-Note
        float lastBendSemitones = 0.0f;
        float lastPressure01    = 0.5f;
    };

    static constexpr uint32_t kDroneFingerBase = 0x20000u;
    static constexpr int   kDroneTapMaxMs        = 250;
    static constexpr float kDroneTapTolerancePx  = 8.0f;

    [[nodiscard]] DroneSun* droneById (uint32_t voiceFingerId) noexcept;
    [[nodiscard]] int droneIndexAt (juce::Point<float> positionPx) const noexcept;

    /** Latched Sonne (Akkord-Abruf): Pixel-Positionen wie die Live-Kreise,
        note = -1, wenn die Sonne außerhalb des Rasters lag (kein noteOn). */
    struct LatchedSun
    {
        juce::Point<float> centre;
        juce::Point<float> orbitOffset;
        bool hasOrbit = false;
        uint32_t fingerId = 0;
        float startNormX = 0.0f;
        float startNormY = 0.0f;
        int note = -1;
    };

    // Synthetische fingerIds der latched Sonnen — kollidiert nie mit
    // Touch-Ids (= sourceIndex + 1, kleine Werte).
    static constexpr uint32_t kLatchedFingerBase = 0x10000u;

    [[nodiscard]] juce::Point<float> normalisedPosition (juce::Point<float> positionPx) const noexcept;
    [[nodiscard]] static int fingerIdFor (const juce::MouseEvent& event) noexcept;

    grid::GridVoiceEngine& engine;
    grid::PadGridLayout    layout;
    grid::RingTouchModel   ring;

    // Gecachte Basiswerte fuer die Sensitivity-/Range-Regler (Block A2/A3) --
    // vor jeder Skalierung MULTIPLIZIEREN, nie den aktuellen Config-Wert
    // weiterskalieren.
    const float baseYRangeNorm;
    const float baseSemitonesPerPadWidth;
    const float baseRingMinPx;
    const float baseRingSpanPx;
    const int   baseLowestNote;

    int       scaleRootNote = 0;
    ScaleType sessionScale  = ScaleType::chromatic;

    grid::InTuneLocation inTuneLocation = grid::InTuneLocation::pad;   // Block B1, Default Pad
    int octaveShiftValue = 0;   // Block D2, in Oktaven (nicht Halbtoenen)

    std::map<int, FingerState> fingers;
    std::vector<LatchedSun> latched;   // Akkord-Speicher-Abruf (leer = keiner)

    // Block M: fingerlos klingende Drone-Sonnen + laufende Id-Vergabe.
    std::vector<DroneSun> drones;
    uint32_t nextDroneFingerId = kDroneFingerBase;

    // Noten-Echo (Block H4): Stärke pro MIDI-Note (0 = aus), Farbe = Fokus-Track.
    std::array<float, 128> echoVelocity {};
    juce::Colour echoColour { 0xffaaaaaa };

    // Block I: Root-Pad-Farbe (padRoot oder Fokus-Track-Farbe) — gesetzt im
    // Ctor (padBaseColour bleibt die pure Referenz für die Grau-Stufen).
    juce::Colour rootPadColour;

    // Block J1/J2: Gravity-Simulation + Settings-Poll (Message-Thread-Puls
    // via VBlank, Masterplan "Message-Thread-Timer").
    void gravityTick();
    [[nodiscard]] bool gravityEnabled() const noexcept
    {
        return panelSettings != nullptr && panelSettings->isGridGravityEnabled();
    }
    /** Effektiver Pitch-Rohwert (normX) eines Fingers — Touch-X, solange
        der Magnet nicht greift oder Gravity aus ist. */
    [[nodiscard]] float effectiveNormX (int fingerId, float touchNormX) const noexcept;

    grid::GridGravity gravity;
    const GridPanelSettings* panelSettings = nullptr;
    double lastGravityTickMs = 0.0;

    // Letzter Member: tickt erst nach vollständiger Konstruktion.
    juce::VBlankAttachment gravityVBlank { this, [this] { gravityTick(); } };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GridKeyboardComponent)
};

} // namespace conduit
