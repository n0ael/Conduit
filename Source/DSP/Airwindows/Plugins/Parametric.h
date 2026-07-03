/* ========================================
 *  Parametric - Parametric.h
 *  Copyright (c) Airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Parametric —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** Parametric — Stacked-Biquad-3-Band-EQ (High/HighMid/LowMid) mit
    Nonlin/Reso-Reglern pro Band und globalem Dry/Wet. */
class Parametric final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Tr Freq
        kParamB = 1, // Treble
        kParamC = 2, // Tr Reso
        kParamD = 3, // HM Freq
        kParamE = 4, // HighMid
        kParamF = 5, // HM Reso
        kParamG = 6, // LM Freq
        kParamH = 7, // LowMid
        kParamI = 8, // LM Reso
        kParamJ = 9, // Dry/Wet
        kNumParameters = 10
    };

    Parametric() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    enum BiqsIndex
    {
        biqs_freq, biqs_reso, biqs_level,
        biqs_nonlin, biqs_temp, biqs_dis,
        biqs_a0, biqs_a1, biqs_b1, biqs_b2,
        biqs_c0, biqs_c1, biqs_d1, biqs_d2,
        biqs_e0, biqs_e1, biqs_f1, biqs_f2,
        biqs_aL1, biqs_aL2, biqs_aR1, biqs_aR2,
        biqs_cL1, biqs_cL2, biqs_cR1, biqs_cR2,
        biqs_eL1, biqs_eL2, biqs_eR1, biqs_eR2,
        biqs_outL, biqs_outR, biqs_total
    };

    double high[biqs_total] {};
    double hmid[biqs_total] {};
    double lmid[biqs_total] {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Parametric)
};

} // namespace conduit::airwindows
