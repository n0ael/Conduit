/* ========================================
 *  Console0Buss - Console0Buss.h
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Console0Buss —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** Console0Buss — Konsolen-Summierungs-Modell (Vol/Pan, Bitshift-Gain-
    Quantisierung + BigFastArcSin-Sättigung), zwei Averaging-Stufen als
    Anti-Alias vor/nach der Sättigung. */
class Console0Buss final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Vol
        kParamB = 1, // Pan
        kNumParameters = 2
    };

    Console0Buss() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    double avgAL = 0.0;
    double avgAR = 0.0;
    double avgBL = 0.0;
    double avgBR = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Console0Buss)
};

} // namespace conduit::airwindows
