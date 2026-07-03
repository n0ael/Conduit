/* ========================================
 *  PearEQ - PearEQ.h
 *  Copyright (c) Airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/PearEQ —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** PearEQ — 5-Band-Filterkaskade (High/HMid/Mid/LMid/Bass/Sub-Level-Regler,
    Pear-Filterstufen A..E jeweils gestaffelt in Frequenz). */
class PearEQ final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // High
        kParamB = 1, // HMid
        kParamC = 2, // Mid
        kParamD = 3, // LMid
        kParamE = 4, // Bass
        kParamF = 5, // Sub
        kNumParameters = 6
    };

    PearEQ() noexcept;

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
        prevSampL7, prevSlewL7, prevSampR7, prevSlewR7,
        prevSampL8, prevSlewL8, prevSampR8, prevSlewR8,
        prevSampL9, prevSlewL9, prevSampR9, prevSlewR9,
        pear_max,
        figL, figR, gndL, gndR, slew, freq, levl,
        pear_total
    };

    double pearA[pear_total] {};
    double pearB[pear_total] {};
    double pearC[pear_total] {};
    double pearD[pear_total] {};
    double pearE[pear_total] {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PearEQ)
};

} // namespace conduit::airwindows
