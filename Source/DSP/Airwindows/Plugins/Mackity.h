/* ========================================
 *  Mackity - Mackity.h
 *  Created 8/12/11 by SPIAdmin
 *  Copyright (c) 2011 __MyCompanyName__, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Mackity —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** Mackity — Konsolen-Emulation (In-Trim/Out-Pad) mit zwei DF1-Biquads
    (19160-Hz-Tiefpass vor/nach einem Fünftgrad-Waveshaper) und zwei
    IIR-Highpass-Stufen. */
class Mackity final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // In Trim
        kParamB = 1, // Out Pad
        kNumParameters = 2
    };

    Mackity() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    double iirSampleAL = 0.0;
    double iirSampleBL = 0.0;
    double iirSampleAR = 0.0;
    double iirSampleBR = 0.0;
    double biquadA[15] {};
    double biquadB[15] {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Mackity)
};

} // namespace conduit::airwindows
