/* ========================================
 *  Tube2 - Tube2.h
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Tube2 —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** Tube2 — Röhren-Sättigung: Input-Pad, asymmetrischer Sinus-Waveshaper,
    Power-Faktor-Sättigung (aus Tube-Parameter), Hysterese/Fuzz-Stufe;
    3x Sample-Averaging-Stufen für hohe Samplerates (overallscale > 1.9). */
class Tube2 final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Input
        kParamB = 1, // Tube
        kNumParameters = 2
    };

    Tube2() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    double previousSampleA = 0.0;
    double previousSampleB = 0.0;
    double previousSampleC = 0.0;
    double previousSampleD = 0.0;
    double previousSampleE = 0.0;
    double previousSampleF = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Tube2)
};

} // namespace conduit::airwindows
