/* ========================================
 *  PointyGuitar - PointyGuitar.h
 *  Copyright (c) Airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/PointyGuitar —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** PointyGuitar — Drive-Kaskade (Poles-Anzahl aus Drive), 4-Band-Speaker-
    Voicing (High/Mid/Low/Sub), H/L-Speaker-Poles, Noise-Gate (Zero-Cross-
    getriggert) und Output. */
class PointyGuitar final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Drive
        kParamB = 1, // Presnce
        kParamC = 2, // High
        kParamD = 3, // Mid
        kParamE = 4, // Low
        kParamF = 5, // Sub
        kParamG = 6, // HSpeakr
        kParamH = 7, // LSpeakr
        kParamI = 8, // Gate
        kParamJ = 9, // Output
        kNumParameters = 10
    };

    PointyGuitar() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    double angSL[18][12] {};
    double angAL[18][12] {};
    double iirHPositionL[37] {};
    double iirHAngleL[37] {};
    double iirBPositionL[37] {};
    double iirBAngleL[37] {};
    bool WasNegativeL = false;
    int ZeroCrossL = 0;
    double gaterollerL = 0.0;
    double gateL = 0.0;

    double angSR[18][12] {};
    double angAR[18][12] {};
    double iirHPositionR[37] {};
    double iirHAngleR[37] {};
    double iirBPositionR[37] {};
    double iirBAngleR[37] {};
    bool WasNegativeR = false;
    int ZeroCrossR = 0;
    double gaterollerR = 0.0;
    double gateR = 0.0;

    double angG[12] {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PointyGuitar)
};

} // namespace conduit::airwindows
