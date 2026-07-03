/* ========================================
 *  TapeHack2 - TapeHack2.h
 *  Copyright (c) Airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/TapeHack2 —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** TapeHack2 — Tape-Sättigung mit gestaffelten Moving-Average-Filtern
    (2/4/8/16/32 Sample-Fenster vor UND nach dem Waveshaper) plus
    Slew-abhängiger Blend-Hüllkurve ("dark"/"depth of filter"). */
class TapeHack2 final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Input
        kParamB = 1, // Output
        kParamC = 2, // Dry/Wet
        kNumParameters = 3
    };

    TapeHack2() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    std::array<double, 33> avg32L {};
    std::array<double, 33> avg32R {};
    std::array<double, 17> avg16L {};
    std::array<double, 17> avg16R {};
    std::array<double, 9>  avg8L {};
    std::array<double, 9>  avg8R {};
    std::array<double, 5>  avg4L {};
    std::array<double, 5>  avg4R {};
    std::array<double, 3>  avg2L {};
    std::array<double, 3>  avg2R {};
    std::array<double, 33> post32L {};
    std::array<double, 33> post32R {};
    std::array<double, 17> post16L {};
    std::array<double, 17> post16R {};
    std::array<double, 9>  post8L {};
    std::array<double, 9>  post8R {};
    std::array<double, 5>  post4L {};
    std::array<double, 5>  post4R {};
    std::array<double, 3>  post2L {};
    std::array<double, 3>  post2R {};
    double lastDarkL = 0.0;
    double lastDarkR = 0.0;
    int avgPos = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TapeHack2)
};

} // namespace conduit::airwindows
