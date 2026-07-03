/* ========================================
 *  Pockey2 - Pockey2.h
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Pockey2 —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** Pockey2 — uLaw-Bitcrusher/Sample-Rate-Reducer ("DeFreq"/"DeRez") mit
    Dry/Wet. */
class Pockey2 final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // DeFreq
        kParamB = 1, // DeRez
        kParamC = 2, // Dry/Wet
        kNumParameters = 3
    };

    Pockey2() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    double lastSampleL = 0.0;
    double heldSampleL = 0.0;
    double previousHeldL = 0.0;

    double lastSampleR = 0.0;
    double heldSampleR = 0.0;
    double previousHeldR = 0.0;

    double position = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Pockey2)
};

} // namespace conduit::airwindows
