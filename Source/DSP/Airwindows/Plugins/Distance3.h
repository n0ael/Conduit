/* ========================================
 *  Distance3 - Distance3.h
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Distance3 —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** Distance3 — dreistufiger Soft-Slew + "Air Discontinuity"-Effekt der
    akustische Entfernung simuliert (Distance, Top dB, Dry/Wet). Nur
    Fixed-Size-Ringpuffer (dscBuf = 90 je Kanal/Stufe), kein Lookahead,
    kein Heap im Verarbeitungspfad. */
class Distance3 final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Distance
        kParamB = 1, // Top dB
        kParamC = 2, // Dry/Wet
        kNumParameters = 3
    };

    Distance3() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    static constexpr int dscBuf = 90;

    double lastclampAL = 0.0, clampAL = 0.0, changeAL = 0.0, prevresultAL = 0.0, lastAL = 0.0;
    double lastclampBL = 0.0, clampBL = 0.0, changeBL = 0.0, prevresultBL = 0.0, lastBL = 0.0;
    double lastclampCL = 0.0, clampCL = 0.0, changeCL = 0.0, prevresultCL = 0.0, lastCL = 0.0;

    double dBaL[dscBuf + 5] {};
    double dBaPosL = 0.0;
    int dBaXL = 1;

    double dBbL[dscBuf + 5] {};
    double dBbPosL = 0.0;
    int dBbXL = 1;

    double dBcL[dscBuf + 5] {};
    double dBcPosL = 0.0;
    int dBcXL = 1;

    double lastclampAR = 0.0, clampAR = 0.0, changeAR = 0.0, prevresultAR = 0.0, lastAR = 0.0;
    double lastclampBR = 0.0, clampBR = 0.0, changeBR = 0.0, prevresultBR = 0.0, lastBR = 0.0;
    double lastclampCR = 0.0, clampCR = 0.0, changeCR = 0.0, prevresultCR = 0.0, lastCR = 0.0;

    double dBaR[dscBuf + 5] {};
    double dBaPosR = 0.0;
    int dBaXR = 1;

    double dBbR[dscBuf + 5] {};
    double dBbPosR = 0.0;
    int dBbXR = 1;

    double dBcR[dscBuf + 5] {};
    double dBcPosR = 0.0;
    int dBcXR = 1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Distance3)
};

} // namespace conduit::airwindows
