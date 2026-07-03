/* ========================================
 *  SmoothEQ3 - SmoothEQ3.h
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/SmoothEQ3 —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** SmoothEQ3 — 3-Band-Shelving-EQ (High/Mid/Bass) über zwei kaskadierte
    Crossover-Stufen (Q==1-Biquad + exponentieller IIR), sehr CPU-günstig. */
class SmoothEQ3 final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // High
        kParamB = 1, // Mid
        kParamC = 2, // Bass
        kNumParameters = 3
    };

    SmoothEQ3() noexcept;

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
        biq_sL1,
        biq_sL2,
        biq_sR1,
        biq_sR2,
        biq_total
    };

    double highFast[biq_total] {};
    double lowFast[biq_total] {};
    double highFastLIIR = 0.0;
    double highFastRIIR = 0.0;
    double lowFastLIIR = 0.0;
    double lowFastRIIR = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SmoothEQ3)
};

} // namespace conduit::airwindows
