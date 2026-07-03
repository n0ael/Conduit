/* ========================================
 *  Console0Channel - Console0Channel.h
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Console0Channel —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** Console0Channel — Konsolen-Kanalzug-Modell (Vol/Pan, Bitshift-Gain-
    Quantisierung + BigFastSin-Sättigung), zwei Averaging-Stufen als
    Anti-Alias vor/nach der Sättigung. */
class Console0Channel final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Vol
        kParamB = 1, // Pan
        kNumParameters = 2
    };

    Console0Channel() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    double avgAL = 0.0;
    double avgAR = 0.0;
    double avgBL = 0.0;
    double avgBR = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Console0Channel)
};

} // namespace conduit::airwindows
