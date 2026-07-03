/* ========================================
 *  DeBess - DeBess.h
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/DeBess —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** DeBess — De-Esser über Slew-der-Slew-Sensing (Intense/Sharp/Depth/Filter/
    Sense), Sense-Monitoring (E > 0.5 hört die entfernte Ess-Energie ab).
    Zwei alternierende IIR-/Ratio-Zustände (flip) glätten Blockgrenzen. */
class DeBess final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Intense
        kParamB = 1, // Sharp
        kParamC = 2, // Depth
        kParamD = 3, // Filter
        kParamE = 4, // Sense
        kNumParameters = 5
    };

    DeBess() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    double sL[41] {}, mL[41] {}, cL[41] {};
    double ratioAL = 1.0;
    double ratioBL = 1.0;
    double iirSampleAL = 0.0;
    double iirSampleBL = 0.0;

    double sR[41] {}, mR[41] {}, cR[41] {};
    double ratioAR = 1.0;
    double ratioBR = 1.0;
    double iirSampleAR = 0.0;
    double iirSampleBR = 0.0;

    bool flip = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeBess)
};

} // namespace conduit::airwindows
