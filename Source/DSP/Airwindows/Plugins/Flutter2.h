/* ========================================
 *  Flutter2 - Flutter2.h
 *  Created 8/12/11 by SPIAdmin
 *  Copyright (c) Airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Flutter2 —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** Flutter2 — Tape-Flutter-Simulation via modulierten Doppler-Delay-Tap
    (1000-Sample-Ringpuffer je Kanal, fest dimensioniert). */
class Flutter2 final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Flutter
        kParamB = 1, // Speed
        kParamC = 2, // Dry/Wet
        kNumParameters = 3
    };

    Flutter2() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    static constexpr int kBufferSize = 1002;

    double dL[kBufferSize] {};
    double dR[kBufferSize] {};
    double sweepL = 0.0;
    double sweepR = 0.0;
    double nextmaxL = 0.5;
    double nextmaxR = 0.5;

    int gcount = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Flutter2)
};

} // namespace conduit::airwindows
