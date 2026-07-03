/* ========================================
 *  Vibrato - Vibrato.h
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Vibrato —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** Vibrato — Delay-Line-Modulation (Speed/Depth) mit sekundärer
    Through-Zero-FM-Modulation der Sweep-Phase (FMSpeed/FMDepth) und
    "Air"-Hochton-Kompensation vor der Interpolation. Delay-Puffer ist
    im Original bereits fest 16386 Samples groß (keine Laufzeit-
    Allokation) — 1:1 als std::array übernommen. */
class Vibrato final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Speed
        kParamB = 1, // Depth
        kParamC = 2, // FMSpeed
        kParamD = 3, // FMDepth
        kParamE = 4, // Inv/Wet
        kNumParameters = 5
    };

    Vibrato() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    std::array<double, 16386> pL {}; // this is processed, not raw incoming samples
    std::array<double, 16386> pR {}; // this is processed, not raw incoming samples
    double sweep = 3.141592653589793238 / 2.0;
    double sweepB = 3.141592653589793238 / 2.0;
    int gcount = 0;

    double airPrevL = 0.0;
    double airEvenL = 0.0;
    double airOddL = 0.0;
    double airFactorL = 0.0;
    double airPrevR = 0.0;
    double airEvenR = 0.0;
    double airOddR = 0.0;
    double airFactorR = 0.0;

    bool flip = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Vibrato)
};

} // namespace conduit::airwindows
