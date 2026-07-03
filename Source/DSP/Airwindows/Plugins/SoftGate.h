/* ========================================
 *  SoftGate - SoftGate.h
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/SoftGate —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** SoftGate — Differenz-basiertes Gate: verfolgt Sample-zu-Sample-Diffs,
    hält bei Überschreiten des Thresholds offen, faded sonst weich zu. */
class SoftGate final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Thresh
        kParamB = 1, // Darken
        kParamC = 2, // Silence
        kNumParameters = 3
    };

    SoftGate() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    double storedL[2] {};
    double diffL = 0.0;
    double storedR[2] {};
    double diffR = 0.0;
    double gate = 1.1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoftGate)
};

} // namespace conduit::airwindows
