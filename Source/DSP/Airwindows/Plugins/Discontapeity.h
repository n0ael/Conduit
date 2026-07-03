/* ========================================
 *  Discontapeity - Discontapeity.h
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Discontapeity —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** Discontapeity — Tape-Diskontinuitäts-/Sättigungseffekt mit internem
    Fixed-Size-Ringpuffer (dscBuf = 256, kein Heap, Message-Thread-
    Konstruktion) plus Taylor-Serien-Sättigung ("TapeHack"). Ein Parameter
    "More". */
class Discontapeity final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // More
        kNumParameters = 1
    };

    Discontapeity() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    static constexpr int dscBuf = 256;

    double dBaL[dscBuf + 5] {};
    double dBaPosL = 0.0;
    int dBaXL = 1;

    double dBaR[dscBuf + 5] {};
    double dBaPosR = 0.0;
    int dBaXR = 1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Discontapeity)
};

} // namespace conduit::airwindows
