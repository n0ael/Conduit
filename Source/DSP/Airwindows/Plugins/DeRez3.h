/* ========================================
 *  DeRez3 - DeRez3.h
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/DeRez3 —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** DeRez3 — Sample-Rate-/Bit-Reducer mit Bezier-Kurven-Rekonstruktion.
    Parameter werden intern pro Block linear interpoliert (rezA/rezB etc.,
    Original-Verhalten aus processReplacing). */
class DeRez3 final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Rate
        kParamB = 1, // Rez
        kParamC = 2, // Dry/Wet
        kNumParameters = 3
    };

    DeRez3() noexcept;

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
        bez_cycle,
        bez_total
    };
    double bez[bez_total] {};

    double rezA = 1.0, rezB = 1.0;
    double bitA = 1.0, bitB = 1.0;
    double wetA = 1.0, wetB = 1.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeRez3)
};

} // namespace conduit::airwindows
