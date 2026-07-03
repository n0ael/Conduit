/* ========================================
 *  Sweeten - Sweeten.h
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Sweeten —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** Sweeten — entfernt gefiltertes 2nd-Harmonic-Aliasing (Undersampling-
    Averaging-Kaskade vor/nach der Nichtlinearität), ein Parameter steuert
    die Nichtlinearitäts-Bit-Tiefe. */
class Sweeten final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Sweeten
        kNumParameters = 1
    };

    Sweeten() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    double savg[20] {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Sweeten)
};

} // namespace conduit::airwindows
