/* ========================================
 *  Inflamer - Inflamer.h
 *  Created 8/12/11 by SPIAdmin
 *  Copyright (c) Airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Inflamer —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** Inflamer — Zweiwege-Waveshaper (+50/-50-Harmonic-Blend), Drive über eine
    diskrete Bit-Gain-Tabelle, Pan-abhängige Kurvenwahl je Kanal. Zustandslos
    bis auf den fpd-Xorshift. */
class Inflamer final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Drive
        kParamB = 1, // Curve
        kParamC = 2, // Effect
        kNumParameters = 3
    };

    Inflamer() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Inflamer)
};

} // namespace conduit::airwindows
