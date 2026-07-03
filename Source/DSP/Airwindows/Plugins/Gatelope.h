/* ========================================
 *  Gatelope - Gatelope.h
 *  Created 8/12/11 by SPIAdmin
 *  Copyright (c) 2011 __MyCompanyName__, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Gatelope —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** Gatelope — gated, envelope-gefiltertes Stereo-Gate/Transient-Shaper
    (Treble/Bass-Split-IIR, alternierende A/B-Filterbank pro Sample). */
class Gatelope final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Thresh
        kParamB = 1, // TrebSus
        kParamC = 2, // BassSus
        kParamD = 3, // AttackS
        kParamE = 4, // Dry/Wet
        kNumParameters = 5
    };

    Gatelope() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    double iirLowpassAL = 0.0;
    double iirLowpassBL = 0.0;
    double iirHighpassAL = 0.0;
    double iirHighpassBL = 0.0;
    double iirLowpassAR = 0.0;
    double iirLowpassBR = 0.0;
    double iirHighpassAR = 0.0;
    double iirHighpassBR = 0.0;
    double treblefreq = 1.0;
    double bassfreq = 0.0;
    bool flip = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Gatelope)
};

} // namespace conduit::airwindows
