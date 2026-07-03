/* ========================================
 *  GlitchShifter - GlitchShifter.h
 *  Created 8/12/11 by SPIAdmin
 *  Copyright (c) 2011 __MyCompanyName__, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/GlitchShifter —
 *  Zero-Cross-Auswahl, Air-Sektion, Feedback und 24-Bit-Integer-Skala wie im
 *  Original (Chris Johnson / Airwindows, MIT).
 *
 *  BEWUSSTE ABWEICHUNG vom Original (User-Entscheidung 03.07.2026, Klang-
 *  qualität vor 1:1-Treue — das Original knackst hörbar bei Note/Trim/
 *  Tighten-Bewegungen):
 *   - Splices laufen als echter DUAL-TAP-CROSSFADE: zwei Lese-Taps pro
 *     Kanal; erreicht der aktive Tap die Ring-Grenze, friert er dort ein
 *     (an der Dry-Tap-Grenze = nahtloses Live-Signal) und blendet aus,
 *     während der neue Tap an der Splice-Position einblendet (Original:
 *     Ein-Sample-Blend nach hartem Positions-Sprung).
 *   - Tighten-/width-Änderungen tauschen die Ring-Geometrie NUR bei kurz
 *     stummgeschaltetem Wet-Pfad ("Duck": ~1.3 ms runter, Geometrie
 *     wechseln, sanft wieder hoch) — ein Geometrie-Sprung bei offenem
 *     Ausgang ist prinzipbedingt nicht klickfrei machbar.
 *  Bei neutralem Note/Trim (speed == 0) entstehen keine Splices mehr —
 *  reine Delay-Wiedergabe ist per Konstruktion klickfrei. */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** GlitchShifter — zero-cross-synchronisiertes Pitch-Shifting über einen
    rotierenden Ringpuffer, mit Dual-Tap-Splice-Crossfade und geducktem
    Geometrie-Wechsel (siehe Header-Kommentar oben). Alle Puffer fest
    dimensioniert, keine Laufzeit-Allokation. */
class GlitchShifter final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Note
        kParamB = 1, // Trim
        kParamC = 2, // Tighten
        kParamD = 3, // Feedbck
        kParamE = 4, // Dry/Wet
        kNumParameters = 5
    };

    GlitchShifter() noexcept;

    //==========================================================================
    // Diagnose für Tests (Klick-Audit) — nur lesend, kein DSP-Einfluss
    int getDebugGcount() const noexcept       { return gcount; }
    int getDebugWidth() const noexcept        { return currentWidth; }
    double getDebugDuck() const noexcept      { return duckGain; }
    double getDebugTapPosL (int tap) const noexcept { return tapPosL[(size_t) tap]; }
    double getDebugXfadeL() const noexcept    { return xfadeL; }
    int getDebugActiveTapL() const noexcept   { return activeTapL; }

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    static constexpr int kBufferSize = 131076;
    static constexpr int kCrossSize = 258;

    // Splice-Crossfade: Länge folgt der Ringgröße (width/2), geklemmt.
    static constexpr int kMinSpliceFade = 16;
    static constexpr int kMaxSpliceFade = 512;
    // Mindestabstand zwischen zwei Tap-Swaps (verhindert Ping-Pong, wenn
    // die Zero-Cross-Auswahl direkt neben einer Ring-Grenze landet).
    static constexpr int kSwapGuardSamples = 16;
    // Wet-Duck für Geometrie-Wechsel: zügig runter (~2.7 ms — schnell genug
    // für responsives Tighten, langsam genug gegen Bass-Thumps), gemächlich
    // wieder hoch. Der Hold hält den Duck nach einem Wechsel kurz unten —
    // bei kontinuierlichen Tighten-Sweeps (Wechsel im ~Block-Takt) entsteht
    // so EIN sauberes Mute statt schnellem Pegel-Flattern.
    static constexpr double kDuckDownStep = 1.0 / 128.0;
    static constexpr double kDuckUpStep = 1.0 / 256.0;
    static constexpr int kDuckHoldSamples = 512;

    double pL[kBufferSize] {};
    double offsetL[kCrossSize] {};
    double pastzeroL[kCrossSize] {};
    double previousL[kCrossSize] {};
    double thirdL[kCrossSize] {};
    double fourthL[kCrossSize] {}; // Original-Quirk: wird nie beschrieben, aber gelesen
    double tempL = 0.0;
    double lasttempL = 0.0;
    double thirdtempL = 0.0;
    double fourthtempL = 0.0;
    double sincezerocrossL = 0.0;
    int crossesL = 0;
    int realzeroesL = 0;

    // Dual-Tap: zwei Lese-Positionen, aktiver Tap blendet mit xfade -> 1 ein.
    double tapPosL[2] {};
    int activeTapL = 0;
    double xfadeL = 1.0;
    int swapGuardL = 0;

    double airPrevL = 0.0;
    double airEvenL = 0.0;
    double airOddL = 0.0;
    double airFactorL = 0.0;

    double pR[kBufferSize] {};
    double offsetR[kCrossSize] {};
    double pastzeroR[kCrossSize] {};
    double previousR[kCrossSize] {};
    double thirdR[kCrossSize] {};
    double fourthR[kCrossSize] {};
    double tempR = 0.0;
    double lasttempR = 0.0;
    double thirdtempR = 0.0;
    double fourthtempR = 0.0;
    double sincezerocrossR = 0.0;
    int crossesR = 0;
    int realzeroesR = 0;

    double tapPosR[2] {};
    int activeTapR = 0;
    double xfadeR = 1.0;
    int swapGuardR = 0;

    double airPrevR = 0.0;
    double airEvenR = 0.0;
    double airOddR = 0.0;
    double airFactorR = 0.0;

    int gcount = 0;
    bool flip = false;

    // Aktive Ring-Geometrie (width). 0 = uninitialisiert, erster Block
    // übernimmt den Zielwert ohne Duck. Weicht targetWidth (aus Tighten)
    // von currentWidth ab, duckt der Wet-Pfad und die Geometrie wechselt
    // erst bei duckGain == 0.
    int currentWidth = 0;
    double duckGain = 1.0;
    int duckHold = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GlitchShifter)
};

} // namespace conduit::airwindows
