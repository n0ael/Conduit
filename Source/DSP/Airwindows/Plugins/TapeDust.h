/* ========================================
 *  TapeDust - TapeDust.h
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/TapeDust —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *
 *  Original nutzt `rand()/RAND_MAX` MITTEN im processReplacing-Körper (nicht
 *  nur zur fpd-Seed-Initialisierung) — das ist ein RT-Safety-Verstoß gegen
 *  CLAUDE.md 3.1. Ersetzt durch den bereits laufenden fpd-Xorshift der Basis
 *  als Uniform-Random-Quelle (Präzedenzfall Flutter2: `fpdL / (double)
 *  UINT32_MAX`), Xorshift wird VOR jeder Nutzung advanced. Deterministisch,
 *  lock- und allocation-free, kein DSP-Charakterverlust (Original bezieht die
 *  Zufallszahl exakt einmal pro Sample für den Rauschanteil, hier ebenso —
 *  nur die Quelle ist eine andere PRNG-Familie).
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** TapeDust — Tape-Rauschsimulation: pro Sample zufällige Gain-Verteilung
    über ein 9-Tap-History-Fenster, Amplitude moduliert durch Sample-zu-
    Sample-Differenz (xfuzz). */
class TapeDust final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Dust
        kParamB = 1, // Dry/Wet
        kNumParameters = 2
    };

    TapeDust() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    bool fpFlip = true;

    double bL[11] {};
    double fL[11] {};
    double bR[11] {};
    double fR[11] {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TapeDust)
};

} // namespace conduit::airwindows
