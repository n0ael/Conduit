/* ========================================
 *  DigitalBlack - DigitalBlack.h
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/DigitalBlack —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** DigitalBlack — Noise-Gate mit Zero-Crossing-getriebenem "Gateroller"
    (Threshold, Dry/Wet). */
class DigitalBlack final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Thresh
        kParamB = 1, // Dry/Wet
        kNumParameters = 2
    };

    DigitalBlack() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    bool WasNegativeL = false;
    int ZeroCrossL = 0;
    double gaterollerL = 0.0;
    bool WasNegativeR = false;
    int ZeroCrossR = 0;
    double gaterollerR = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DigitalBlack)
};

} // namespace conduit::airwindows
