/* ========================================
 *  Air4 - Air4.h
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Air4 —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** Air4 — Hochfrequenz-"Air"-Boost/Ground-Cancel-Prozessor mit
    parametrischer Sinew-Begrenzung (Air, Gnd, DarkF, Ratio). */
class Air4 final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Air
        kParamB = 1, // Gnd
        kParamC = 2, // DarkF
        kParamD = 3, // Ratio
        kNumParameters = 4
    };

    Air4() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    enum AirIndex
    {
        pvAL1, pvSL1, accSL1, acc2SL1,
        pvAL2, pvSL2, accSL2, acc2SL2,
        pvAL3, pvSL3, accSL3,
        pvAL4, pvSL4,
        gndavgL, outAL, gainAL, lastSL,
        pvAR1, pvSR1, accSR1, acc2SR1,
        pvAR2, pvSR2, accSR2, acc2SR2,
        pvAR3, pvSR3, accSR3,
        pvAR4, pvSR4,
        gndavgR, outAR, gainAR, lastSR,
        air_total
    };
    double air[air_total] {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Air4)
};

} // namespace conduit::airwindows
