#pragma once

#include <cmath>

#include "LooperMath.h"

namespace conduit::looper
{

//==============================================================================
/**
    Pure Clip-Phase-Mathe des Looper-Vollausbaus (Meilenstein M1) — die
    Verallgemeinerung von `loopPhaseBeats` auf Clips mit Varispeed (rate),
    Reverse und ÷2-Fenster (windowOffset). Bewusst JUCE-frei (Muster
    LooperMath): LooperBank, UI und Tests teilen sich exakt dieselbe
    Arithmetik.

    Modell: Die Phase eines Clips ist BEAT-ABGELEITET, kein freilaufender
    Zähler. Statt `loopEndBeat` trägt jeder Clip einen Phasen-ANKER a:

        ph(b) = wrap((b − a) · rate, L)         ∈ [0, L)

    Bei rate = 1 und a = loopEndBeat ist das exakt `loopPhaseBeats` —
    Paritätstest in LooperClipMathTests. Rate-/Reverse-Wechsel werden
    POSITIONS-KONTINUIERLICH ausgeführt, indem der Anker neu gesetzt wird
    (geschlossene Formeln unten); ×2/÷2 ändern NUR L (und ggf. das Fenster),
    der Anker bleibt — die Phase bleibt damit per Konstruktion beat-starr.

    Lese-Position im aufgenommenen Content (Samples ab Content-Anfang):

        readBeat(ph) = windowOffset + (reversed ? wrap(L − ph, L) : ph)
        readPos      = readBeat · samplesPerBeatRecorded

    windowOffset wählt das ÷2-Fenster INNERHALB des committeten Contents
    (User-Option "erste vs. aktuelle Hälfte"); Content-Grenzen prüft der
    Aufrufer (LooperBank kennt contentBeats).
*/

/** wrap in [0, length); length ≤ 0 → 0. */
[[nodiscard]] inline double wrapBeats (double value, double lengthBeats) noexcept
{
    if (lengthBeats <= 0.0)
        return 0.0;

    auto wrapped = std::fmod (value, lengthBeats);
    if (wrapped < 0.0)
        wrapped += lengthBeats;
    return wrapped;
}

/** Beat-abgeleitete Clip-Phase in [0, lengthBeats). */
[[nodiscard]] inline double clipPhaseBeats (double sessionBeat, double anchorBeat,
                                            double rate, double lengthBeats) noexcept
{
    return wrapBeats ((sessionBeat - anchorBeat) * rate, lengthBeats);
}

/** Lese-Beat im Content-Fenster (windowOffset + gerichtete Phase). */
[[nodiscard]] inline double clipReadBeat (double phaseBeats, double lengthBeats,
                                          bool reversed,
                                          double windowOffsetBeats) noexcept
{
    const auto directed = reversed ? wrapBeats (lengthBeats - phaseBeats, lengthBeats)
                                   : phaseBeats;
    return windowOffsetBeats + directed;
}

/** Lese-Position in Samples ab Content-Anfang (fraktional — der Renderer
    interpoliert linear, Muster renderVoiceSample). */
[[nodiscard]] inline double clipReadPosition (double sessionBeat, double anchorBeat,
                                              double rate, double lengthBeats,
                                              bool reversed, double windowOffsetBeats,
                                              double samplesPerBeatRecorded) noexcept
{
    const auto phase = clipPhaseBeats (sessionBeat, anchorBeat, rate, lengthBeats);
    return clipReadBeat (phase, lengthBeats, reversed, windowOffsetBeats)
           * samplesPerBeatRecorded;
}

//==============================================================================
/** Neuer Anker für einen Rate-Wechsel r0 → r1 am Session-Beat b, so dass die
    Phase kontinuierlich bleibt: ph(b) unverändert, danach läuft sie mit r1.
    r1 ≤ 0 → Anker unverändert (Rate 0/negativ ist kein gültiger Zustand). */
[[nodiscard]] inline double reanchorForRateChange (double sessionBeat,
                                                   double oldAnchorBeat,
                                                   double oldRate, double newRate,
                                                   double lengthBeats) noexcept
{
    if (newRate <= 0.0)
        return oldAnchorBeat;

    const auto phase = clipPhaseBeats (sessionBeat, oldAnchorBeat, oldRate, lengthBeats);
    return sessionBeat - phase / newRate;
}

/** Neuer Anker für ein SOFORTIGES Reverse-Toggle am Session-Beat b mit
    Positionsspiegelung (User-Option "sofort"): die LESE-Position bleibt
    exakt stehen, nur die Richtung dreht. Symmetrisch — zweimal anwenden
    stellt den Ausgangszustand wieder her. */
[[nodiscard]] inline double reanchorForReverseToggle (double sessionBeat,
                                                      double anchorBeat, double rate,
                                                      double lengthBeats) noexcept
{
    if (rate <= 0.0)
        return anchorBeat;

    const auto phase    = clipPhaseBeats (sessionBeat, anchorBeat, rate, lengthBeats);
    const auto mirrored = wrapBeats (lengthBeats - phase, lengthBeats);
    return sessionBeat - mirrored / rate;
}

//==============================================================================
/** ÷2-Fenster-Wahl (User-Option, Menü). */
enum class HalveMode : int
{
    firstHalf = 0,   // Fenster bleibt am Content-Anfang stehen
    currentHalf,     // die Hälfte, in der der Playhead gerade steht
};

/** windowOffset nach einem ÷2 (newLength = alte Länge / 2). `currentPhase`
    ist die Phase VOR dem ÷2 (∈ [0, 2·newLength)). firstHalf behält das
    Fenster, currentHalf springt auf die gerade spielende Hälfte —
    die Lese-Kontinuität folgt daraus automatisch (Phase wrappt auf
    ph mod newLength, das Fenster gleicht den Rest aus). */
[[nodiscard]] inline double windowOffsetForHalve (HalveMode mode,
                                                  double oldWindowOffsetBeats,
                                                  double newLengthBeats,
                                                  double currentPhaseBeats) noexcept
{
    if (mode == HalveMode::firstHalf || newLengthBeats <= 0.0)
        return oldWindowOffsetBeats;

    const auto halfIndex = std::floor (currentPhaseBeats / newLengthBeats);
    return oldWindowOffsetBeats + halfIndex * newLengthBeats;
}

//==============================================================================
/** Sample-genauer Grid-Übertritt innerhalb eines Blocks (CLAUDE.md 4.5):
    Index des ersten Samples, dessen Beat ≥ der nächsten Grid-Grenze liegt,
    oder −1, wenn der Block keine Grenze enthält. qBeats ≤ 0 → 0 (sofort am
    Blockanfang). Liegt der Blockanfang EXAKT auf einer Grenze, gehört sie
    diesem Block (der Vorgänger-Block lag komplett davor) → 0. */
[[nodiscard]] inline int gridCrossingOffset (double blockStartBeat,
                                             double beatsPerSample, int numSamples,
                                             double qBeats) noexcept
{
    if (qBeats <= 0.0)
        return 0;
    if (numSamples <= 0 || beatsPerSample <= 0.0)
        return -1;

    // Epsilon absorbiert FP-Rauschen der Beat-Rechnung (Anker + n·beatStep):
    // eine Grenze, die nur um Rundungsfehler "verfehlt" wird, zählt als
    // getroffen — sonst rastet der Start ein Sample (oder eine ganze
    // Grid-Periode) zu spät.
    constexpr double epsilon = 1.0e-9;

    const auto gridIndex = std::ceil (blockStartBeat / qBeats - epsilon);
    const auto boundary  = gridIndex * qBeats;

    auto offset = (boundary - blockStartBeat) / beatsPerSample;
    if (offset < 0.0)
        offset = 0.0;
    if (offset >= static_cast<double> (numSamples))
        return -1;

    return static_cast<int> (std::ceil (offset - epsilon));
}

} // namespace conduit::looper
