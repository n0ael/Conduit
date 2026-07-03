/* ========================================
 *  Dubly3 - Dubly3.h
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Dubly3 —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** Dubly3 — Dolby-artiger Encode/Decode-Sättigungseffekt (Companding um
    einen sin()-Nichtlinearitäts-Kern, Input/Tilt/Shape/Output). */
class Dubly3 final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Input
        kParamB = 1, // Tilt
        kParamC = 2, // Shape
        kParamD = 3, // Output
        kNumParameters = 4
    };

    Dubly3() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    double iirEncL = 0.0, iirEncR = 0.0;
    double iirDecL = 0.0, iirDecR = 0.0;
    double compEncL = 1.0, compEncR = 1.0;
    double compDecL = 1.0, compDecR = 1.0;
    double avgEncL = 0.0, avgEncR = 0.0;
    double avgDecL = 0.0, avgDecR = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Dubly3)
};

} // namespace conduit::airwindows
