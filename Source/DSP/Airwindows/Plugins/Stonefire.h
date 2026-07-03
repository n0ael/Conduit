/* ========================================
 *  Stonefire - Stonefire.h
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Stonefire —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** Stonefire — Air3-Hochton-Extraktion + Kalman-Crossover in ein
    Drei-Band-Gain-Tool (Air/Fire/Stone), Bandgains werden pro Block
    linear von der vorherigen zur neuen Einstellung interpoliert
    (Original-Verhalten: `temp = sampleFrames / inFramesToProcess`). */
class Stonefire final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Air
        kParamB = 1, // Fire
        kParamC = 2, // Stone
        kParamD = 3, // Range
        kNumParameters = 4
    };

    Stonefire() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    enum
    {
        pvAL1, pvSL1, accSL1, acc2SL1,
        pvAL2, pvSL2, accSL2, acc2SL2,
        pvAL3, pvSL3, accSL3,
        pvAL4, pvSL4,
        gndavgL, outAL, gainAL,
        pvAR1, pvSR1, accSR1, acc2SR1,
        pvAR2, pvSR2, accSR2, acc2SR2,
        pvAR3, pvSR3, accSR3,
        pvAR4, pvSR4,
        gndavgR, outAR, gainAR,
        air_total
    };
    double air[air_total] {};

    enum
    {
        prevSampL1, prevSlewL1, accSlewL1,
        prevSampL2, prevSlewL2, accSlewL2,
        prevSampL3, prevSlewL3, accSlewL3,
        kalGainL, kalOutL,
        prevSampR1, prevSlewR1, accSlewR1,
        prevSampR2, prevSlewR2, accSlewR2,
        prevSampR3, prevSlewR3, accSlewR3,
        kalGainR, kalOutR,
        kal_total
    };
    double kal[kal_total] {};

    double trebleGainA = 1.0;
    double trebleGainB = 1.0;
    double midGainA = 1.0;
    double midGainB = 1.0;
    double bassGainA = 1.0;
    double bassGainB = 1.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Stonefire)
};

} // namespace conduit::airwindows
