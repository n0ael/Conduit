/* ========================================
 *  Smooth - Smooth.h
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Smooth —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** Smooth — 5-stufige Slew/Gain-Chase-Kaskade (A..E) pro Kanal mit Makeup-
    Gain und Dry/Wet. */
class Smooth final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Smooth
        kParamB = 1, // Output
        kParamC = 2, // Dry/Wet
        kNumParameters = 3
    };

    Smooth() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    double lastSampleAL = 0.0;
    double gainAL = 1.0;
    double lastSampleBL = 0.0;
    double gainBL = 1.0;
    double lastSampleCL = 0.0;
    double gainCL = 1.0;
    double lastSampleDL = 0.0;
    double gainDL = 1.0;
    double lastSampleEL = 0.0;
    double gainEL = 1.0;

    double lastSampleAR = 0.0;
    double gainAR = 1.0;
    double lastSampleBR = 0.0;
    double gainBR = 1.0;
    double lastSampleCR = 0.0;
    double gainCR = 1.0;
    double lastSampleDR = 0.0;
    double gainDR = 1.0;
    double lastSampleER = 0.0;
    double gainER = 1.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Smooth)
};

} // namespace conduit::airwindows
