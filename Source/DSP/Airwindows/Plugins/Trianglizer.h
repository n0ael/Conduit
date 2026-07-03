/* ========================================
 *  Trianglizer - Trianglizer.h
 *  Copyright (c) Airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Trianglizer —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** Trianglizer — Log/Exp-Waveshaper, der die Wellenform Richtung Dreieck
    (Tri) oder fetter Sättigung (Fat) verschiebt, zustandslos bis auf fpd. */
class Trianglizer final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Tri/Fat
        kParamB = 1, // Dry/Wet
        kNumParameters = 2
    };

    Trianglizer() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Trianglizer)
};

} // namespace conduit::airwindows
