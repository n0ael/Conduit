/* ========================================
 *  TapeDelay2 - TapeDelay2.h
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/TapeDelay2 —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *
 *  Delay-Ringpuffer dL/dR[88211] ist im Original bereits fest dimensioniert
 *  (kein Lazy-Alloc, keine SR-abhängige Neuallokation) — 1:1 als Klassen-
 *  Member übernommen, kein Heap-Alloc im Prozesspfad (CLAUDE.md 3.1).
 *  88200 Samples deckt 2s bei 44.1kHz; delayL/delayR wrappen explizit gegen
 *  88200.0, der Index bleibt innerhalb der Arraygrenze.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** TapeDelay2 — Bandecho mit moduliertem Sample-Rate-Delay (Flutter/Wow),
    Regenerations-Filter (State-Variable) und Ausgangsfilter, submix-artiges
    50/50-Dry/Wet (kein Crossfade). */
class TapeDelay2 final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Time
        kParamB = 1, // Regen
        kParamC = 2, // Freq
        kParamD = 3, // Reso
        kParamE = 4, // Flutter
        kParamF = 5, // Dry/Wet
        kNumParameters = 6
    };

    TapeDelay2() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    static constexpr int kDelaySize = 88211;

    double dL[kDelaySize] {};
    double prevSampleL = 0.0;
    double delayL = 0.0;
    double sweepL = 0.0;
    double regenFilterL[9] {};
    double outFilterL[9] {};
    double lastRefL[10] {};

    double dR[kDelaySize] {};
    double prevSampleR = 0.0;
    double delayR = 0.0;
    double sweepR = 0.0;
    double regenFilterR[9] {};
    double outFilterR[9] {};
    double lastRefR[10] {};

    int cycle = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TapeDelay2)
};

} // namespace conduit::airwindows
