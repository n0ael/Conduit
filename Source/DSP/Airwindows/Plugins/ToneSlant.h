/* ========================================
 *  ToneSlant - ToneSlant.h
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/ToneSlant —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** ToneSlant — FIR-Tiefpass-Subtraktions-EQ: Voicing bestimmt die
    Filterlänge/-tiefe (1..100 Taps), Highs die Anwendungsstärke der
    berechneten Korrektur (-1..+1, negativ = Höhen dämpfen). */
class ToneSlant final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Voicing
        kParamB = 1, // Highs
        kNumParameters = 2
    };

    ToneSlant() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    std::array<double, 102> bL {};
    std::array<double, 102> bR {};
    std::array<double, 102> f {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ToneSlant)
};

} // namespace conduit::airwindows
