/* ========================================
 *  StoneFireComp - StoneFireComp.h
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/StoneFireComp —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** StoneFireComp — Splittet das Signal per Kalman-Filter in "Fire" (schnell)
    und "Stone" (langsam) und komprimiert beide Bänder getrennt. */
class StoneFireComp final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Fire Th
        kParamB = 1, // Attack (Fire)
        kParamC = 2, // Release (Fire)
        kParamD = 3, // Fire
        kParamE = 4, // Stone Th
        kParamF = 5, // Attack (Stone)
        kParamG = 6, // Release (Stone)
        kParamH = 7, // Stone
        kParamI = 8, // Range
        kParamJ = 9, // Ratio
        kNumParameters = 10
    };

    StoneFireComp() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    enum
    {
        prevSampL1, prevSlewL1, accSlewL1,
        prevSampL2, prevSlewL2, accSlewL2,
        prevSampL3, prevSlewL3, accSlewL3,
        kalGainL, kalOutL,
        prevSampR1, prevSlewR1, accSlewR1,
        prevSampR2, prevSlewR2, accSlewR2,
        prevSampR3, prevSlewR3, accSlewR3,
        kalGainR, kalOutR,
        kal_total
    };

    double kal[kal_total] {};
    double fireCompL = 1.0;
    double fireCompR = 1.0;
    double stoneCompL = 1.0;
    double stoneCompR = 1.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StoneFireComp)
};

} // namespace conduit::airwindows
