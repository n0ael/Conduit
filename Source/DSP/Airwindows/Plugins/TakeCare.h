/* ========================================
 *  TakeCare - TakeCare.h
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/TakeCare —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *
 *  Alle Delay-/Reverb-Puffer sind im Original bereits fest dimensioniert
 *  (double a3XL[32767+5], keine Lazy-Allokation) — 1:1 als Klassen-Member
 *  übernommen, kein Heap-Alloc im Prozesspfad (CLAUDE.md 3.1). `buf`
 *  (Ringpuffer-Schreibindex-Obergrenze) ist blockkonstant über targetBuf
 *  auf max. 32766 begrenzt, bleibt also innerhalb der Arraygrenzen.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** TakeCare — Neun-Tap-Bezier-Undersampling-"Reverb"-Diffusionsnetzwerk mit
    Vibrato-Modulation und weichem ClipOnly-Ausgangslimiter. */
class TakeCare final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Speed
        kParamB = 1, // Rando
        kParamC = 2, // Depth
        kParamD = 3, // Regen
        kParamE = 4, // Derez
        kParamF = 5, // Buffer
        kParamG = 6, // Output
        kParamH = 7, // Dry/Wet
        kNumParameters = 8
    };

    TakeCare() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    static constexpr int kBufSize = 32767 + 5;

    double a3AL[kBufSize] {};
    double a3BL[kBufSize] {};
    double a3CL[kBufSize] {};
    double a3DL[kBufSize] {};
    double a3EL[kBufSize] {};
    double a3FL[kBufSize] {};
    double a3GL[kBufSize] {};
    double a3HL[kBufSize] {};
    double a3IL[kBufSize] {};
    double a3AR[kBufSize] {};
    double a3BR[kBufSize] {};
    double a3CR[kBufSize] {};
    double a3DR[kBufSize] {};
    double a3ER[kBufSize] {};
    double a3FR[kBufSize] {};
    double a3GR[kBufSize] {};
    double a3HR[kBufSize] {};
    double a3IR[kBufSize] {};

    int c3AL = 1, c3AR = 1, c3BL = 1, c3BR = 1, c3CL = 1, c3CR = 1, c3DL = 1, c3DR = 1, c3EL = 1, c3ER = 1;
    int c3FL = 1, c3FR = 1, c3GL = 1, c3GR = 1, c3HL = 1, c3HR = 1, c3IL = 1, c3IR = 1;
    double f3AL = 0.0, f3BL = 0.0, f3CL = 0.0, f3CR = 0.0, f3FR = 0.0, f3IR = 0.0;
    double avg3L = 0.0, avg3R = 0.0;

    enum
    {
        bez_AL, bez_AR,
        bez_BL, bez_BR,
        bez_CL, bez_CR,
        bez_InL, bez_InR,
        bez_UnInL, bez_UnInR,
        bez_SampL, bez_SampR,
        bez_AvgInSampL, bez_AvgInSampR,
        bez_AvgOutSampL, bez_AvgOutSampR,
        bez_cycle,
        bez_total
    }; //the new undersampling. bez signifies the bezier curve reconstruction
    double bez[bez_total] {};

    double rotate = 0.0;
    double oldfpd = 0.4294967295;

    int buf = 8192;
    double vibDepth = 0.0;
    double derezA = 0.5;
    double derezB = 0.5;
    double outA = 1.0;
    double outB = 1.0;
    double wetA = 1.0;
    double wetB = 1.0;

    double lastSampleL = 0.0;
    bool wasPosClipL = false;
    bool wasNegClipL = false;
    double lastSampleR = 0.0;
    bool wasPosClipR = false;
    bool wasNegClipR = false; //Stereo ClipOnly

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TakeCare)
};

} // namespace conduit::airwindows
