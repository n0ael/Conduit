/* ========================================
 *  DubSub2 - DubSub2.h
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/DubSub2 —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** DubSub2 — "HeadBump"-Sub-Sättigung: kubische Waveshaper-Rückkopplung
    gefolgt von einer kaskadierten Biquad-Bandpass-Filterstufe (aus
    ZBandpass übernommen). Kein Lookahead, kein Heap. */
class DubSub2 final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // HeadBmp
        kParamB = 1, // HeadFrq
        kParamC = 2, // Dry/Wet
        kNumParameters = 3
    };

    DubSub2() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    double headBumpL = 0.0;
    double headBumpR = 0.0;

    enum HdbIndex
    {
        hdb_freq,
        hdb_reso,
        hdb_a0,
        hdb_a1,
        hdb_a2,
        hdb_b1,
        hdb_b2,
        hdb_sL1,
        hdb_sL2,
        hdb_sR1,
        hdb_sR2,
        hdb_total
    };
    double hdbA[hdb_total] {};
    double hdbB[hdb_total] {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DubSub2)
};

} // namespace conduit::airwindows
