/* ========================================
 *  Isolator3 - Isolator3.h
 *  Created 8/12/11 by SPIAdmin
 *  Copyright (c) Airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Isolator3 —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** Isolator3 — 7-fach kaskadierte, koeffizienten-interpolierende Biquad-
    Bank (Tiefpass "hiquad" + Hochpass-Differenz "biquad") als Crossover-
    Isolator, plus sample-rate-abhängigem Clip-Delay-Ausgleich
    (ClipOnly2 + intermediate-Puffer, max. Spacing 16). */
class Isolator3 final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Iso
        kParamB = 1, // Q
        kNumParameters = 2
    };

    Isolator3() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    enum
    {
        biq_freq,
        biq_reso,
        biq_a0,
        biq_a1,
        biq_a2,
        biq_b1,
        biq_b2,
        biq_aA0,
        biq_aA1,
        biq_aA2,
        biq_bA1,
        biq_bA2,
        biq_aB0,
        biq_aB1,
        biq_aB2,
        biq_bB1,
        biq_bB2,
        biq_sL1,
        biq_sL2,
        biq_sR1,
        biq_sR2,
        biq_total
    }; // coefficient interpolating biquad filter, stereo

    double biquadA[biq_total] {};
    double biquadB[biq_total] {};
    double biquadC[biq_total] {};
    double biquadD[biq_total] {};
    double biquadE[biq_total] {};
    double biquadF[biq_total] {};
    double biquadG[biq_total] {};
    double hiquadA[biq_total] {};
    double hiquadB[biq_total] {};
    double hiquadC[biq_total] {};
    double hiquadD[biq_total] {};
    double hiquadE[biq_total] {};
    double hiquadF[biq_total] {};
    double hiquadG[biq_total] {};

    // Original-Puffer war [16], aber spacing (Original-Var. `spacing`,
    // aus overallscale, geclampt 1..16) indiziert intermediate[spacing] —
    // bei spacing==16 ein Off-by-one-Schreibzugriff auf intermediate[16]
    // im Original-VST (undefiniertes Verhalten, dort "zufällig" durch
    // Member-Layout überdeckt). Port nutzt [17], um denselben
    // Algorithmus ohne UB/ASan-Fund abzubilden (CLAUDE.md 3.1/13.4) —
    // Wert an Index 16 wird nie sinnvoll gelesen, reines Sicherheitspolster.
    double lastSampleL = 0.0;
    double intermediateL[17] {};
    bool wasPosClipL = false;
    bool wasNegClipL = false;
    double lastSampleR = 0.0;
    double intermediateR[17] {};
    bool wasPosClipR = false;
    bool wasNegClipR = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Isolator3)
};

} // namespace conduit::airwindows
