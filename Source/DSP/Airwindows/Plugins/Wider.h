/* ========================================
 *  Wider - Wider.h
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Wider —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** Wider — Mid/Side-Breite mit unabhängiger Sinus-Sättigung ("Density")
    für Mid und Side sowie einer Ein-Sample-Delay-Interpolation
    (fester Ringpuffer, 4099 Samples) zum Zeit-/Phasen-Ausgleich
    zwischen Mid und Side abhängig von deren Density-Differenz. */
class Wider final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Width
        kParamB = 1, // Center
        kParamC = 2, // Dry/Wet
        kNumParameters = 3
    };

    Wider() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    std::array<double, 4099> p {};
    int count = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Wider)
};

} // namespace conduit::airwindows
