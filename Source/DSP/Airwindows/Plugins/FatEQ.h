/* ========================================
 *  FatEQ - FatEQ.h
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/FatEQ —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** FatEQ — dreibandiges "Pear"-Filterbank-EQ (High/HMid/LMid/Bass) mit
    Taylor-Serien-Sättigung pro Band und ClipOnly2-Ausgangsbegrenzer
    (Fixed-Size-Latenzpuffer intermediateL/R[16], Spacing 1..16 aus der
    Samplerate abgeleitet — kein Heap, kein Lookahead). */
class FatEQ final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // High
        kParamB = 1, // HMid
        kParamC = 2, // LMid
        kParamD = 3, // Bass
        kParamE = 4, // Out
        kNumParameters = 5
    };

    FatEQ() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    enum PearIndex
    {
        prevSampL1, prevSlewL1, prevSampR1, prevSlewR1,
        prevSampL2, prevSlewL2, prevSampR2, prevSlewR2,
        prevSampL3, prevSlewL3, prevSampR3, prevSlewR3,
        prevSampL4, prevSlewL4, prevSampR4, prevSlewR4,
        prevSampL5, prevSlewL5, prevSampR5, prevSlewR5,
        prevSampL6, prevSlewL6, prevSampR6, prevSlewR6,
        pear_max,
        figL,
        figR,
        gndL,
        gndR,
        slew,
        freq,
        levl,
        pear_total
    }; //new pear filter

    double pearA[pear_total] {};
    double pearB[pear_total] {};
    double pearC[pear_total] {};

    // Original-Größe 16 (Airwindows), hier 17: "spacing" wird auf 1..16
    // geklemmt und dann als intermediateL[spacing] indiziert — beim
    // Original-Grenzwert 16 ist das ein Off-by-one auf einem 16-Element-
    // Array (nur bei overallscale >= 16 erreichbar, d.h. Samplerate
    // >= 705600 Hz bei 44100 Hz Basis — außerhalb des Conduit-Zielbereichs,
    // aber ein UB-Fix ohne DSP-Änderung für alle real erreichbaren Werte).
    double lastSampleL = 0.0;
    double intermediateL[17] {};
    bool wasPosClipL = false;
    bool wasNegClipL = false;
    double lastSampleR = 0.0;
    double intermediateR[17] {};
    bool wasPosClipR = false;
    bool wasNegClipR = false; //Stereo ClipOnly2

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FatEQ)
};

} // namespace conduit::airwindows
