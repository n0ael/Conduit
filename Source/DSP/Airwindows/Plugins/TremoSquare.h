/* ========================================
 *  TremoSquare - TremoSquare.h
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/TremoSquare —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** TremoSquare — Tremolo mit Rechteck-Hüllkurve: mutet abwechselnd ganze
    Halbwellen basierend auf Polarität + freilaufendem Oszillator. */
class TremoSquare final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Freq
        kParamB = 1, // Dry/Wet
        kNumParameters = 2
    };

    TremoSquare() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    double osc = 0.0;
    bool polarityL = false;
    bool muteL = false;
    bool polarityR = false;
    bool muteR = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TremoSquare)
};

} // namespace conduit::airwindows
