/* ========================================
 *  Weight - Weight.h
 *  Copyright (c) Airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Weight —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** Weight — Bass-Boost via 8-fach kaskadiertem Holt-Trend-Glätter
    (double-exponential smoothing), additiv auf das Trockensignal
    gemischt. Freq bestimmt die Eckfrequenz, Weight die Boost-Menge
    UND die Resonanz (beta aus resControl abgeleitet). */
class Weight final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Freq
        kParamB = 1, // Weight
        kNumParameters = 2
    };

    Weight() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    std::array<double, 9> previousSampleL {};
    std::array<double, 9> previousTrendL {};
    std::array<double, 9> previousSampleR {};
    std::array<double, 9> previousTrendR {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Weight)
};

} // namespace conduit::airwindows
