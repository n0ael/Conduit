/* ========================================
 *  DeBez - DeBez.h
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/DeBez —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** DeBez — kombinierter Bitcrusher/Sample-Rate-Reducer mit Bezier-Kurven-
    Rekonstruktion ("DeBez" = Rate, "DeRez" = Bit-Tiefe, "Inv/Wet" = Dry/Wet
    inkl. Phasen-Invertierung). Parameter werden intern pro Block linear
    interpoliert (rezA/rezB etc., Original-Verhalten aus processReplacing). */
class DeBez final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // DeBez (Rate)
        kParamB = 1, // DeRez (Bit-Tiefe)
        kParamC = 2, // Inv/Wet
        kNumParameters = 3
    };

    DeBez() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    enum BezIndex
    {
        bez_AL,
        bez_BL,
        bez_CL,
        bez_InL,
        bez_UnInL,
        bez_SampL,
        bez_AR,
        bez_BR,
        bez_CR,
        bez_InR,
        bez_UnInR,
        bez_SampR,
        bez_AvgInSampL,
        bez_AvgInSampR,
        bez_AvgOutSampL,
        bez_AvgOutSampR,
        bez_cycle,
        bez_total
    };
    double bezF[bez_total] {};

    double rezA = 0.5, rezB = 0.5;
    double bitA = 0.5, bitB = 0.5;
    double wetA = 1.0, wetB = 1.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeBez)
};

} // namespace conduit::airwindows
