/* ========================================
 *  CansAW - CansAW.h
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/CansAW —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** CansAW — Headphone-Reverb-Simulation, feste "94 seat room"-Raumgröße
    (Original-Preset 3), 1 Parameter "seatPos" (Bypass/UpFront/SitBack/
    Hallway wählt intern feste Console/Damping/Wet-Presets). */
class CansAW final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kSeatPosParam = 0,
        kNumParameters = 1
    };

    CansAW() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    static constexpr int kShortA = 193;
    static constexpr int kShortB = 464;
    static constexpr int kShortC = 422;
    static constexpr int kShortD = 36;
    static constexpr int kShortE = 166;
    static constexpr int kShortF = 427;
    static constexpr int kShortG = 162;
    static constexpr int kShortH = 476;
    static constexpr int kShortI = 23;
    static constexpr int kShortJ = 345;
    static constexpr int kShortK = 298;
    static constexpr int kShortL = 296;
    static constexpr int kShortM = 12;
    static constexpr int kShortN = 989;
    static constexpr int kShortO = 65;
    static constexpr int kShortP = 484;

    double aAL[kShortA + 5] {}, aBL[kShortB + 5] {}, aCL[kShortC + 5] {}, aDL[kShortD + 5] {};
    double aEL[kShortE + 5] {}, aFL[kShortF + 5] {}, aGL[kShortG + 5] {}, aHL[kShortH + 5] {};
    double aIL[kShortI + 5] {}, aJL[kShortJ + 5] {}, aKL[kShortK + 5] {}, aLL[kShortL + 5] {};
    double aML[kShortM + 5] {}, aNL[kShortN + 5] {}, aOL[kShortO + 5] {}, aPL[kShortP + 5] {};

    double aAR[kShortA + 5] {}, aBR[kShortB + 5] {}, aCR[kShortC + 5] {}, aDR[kShortD + 5] {};
    double aER[kShortE + 5] {}, aFR[kShortF + 5] {}, aGR[kShortG + 5] {}, aHR[kShortH + 5] {};
    double aIR[kShortI + 5] {}, aJR[kShortJ + 5] {}, aKR[kShortK + 5] {}, aLR[kShortL + 5] {};
    double aMR[kShortM + 5] {}, aNR[kShortN + 5] {}, aOR[kShortO + 5] {}, aPR[kShortP + 5] {};

    double feedbackAL = 0.0, feedbackBL = 0.0, feedbackCL = 0.0, feedbackDL = 0.0;
    double feedbackAR = 0.0, feedbackBR = 0.0, feedbackCR = 0.0, feedbackDR = 0.0;

    double iirInL = 0.0, iirFAL = 0.0, iirFBL = 0.0, iirFCL = 0.0, iirFDL = 0.0, iirOutL = 0.0;
    double iirInR = 0.0, iirFAR = 0.0, iirFBR = 0.0, iirFCR = 0.0, iirFDR = 0.0, iirOutR = 0.0;

    int countA = 1, countB = 1, countC = 1, countD = 1, countE = 1, countF = 1, countG = 1, countH = 1;
    int countI = 1, countJ = 1, countK = 1, countL = 1, countM = 1, countN = 1, countO = 1, countP = 1;

    // feste "94 seat room"-Größe (Original-Konstruktor-Default, entspricht Cans-Preset 3)
    int shortA = 192, shortB = 463, shortC = 420, shortD = 34, shortE = 161, shortF = 426, shortG = 160, shortH = 474;
    int shortI = 21, shortJ = 343, shortK = 296, shortL = 294, shortM = 11, shortN = 987, shortO = 64, shortP = 482;

    enum BezIndex
    {
        bez_AL, bez_AR, bez_BL, bez_BR, bez_CL, bez_CR,
        bez_InL, bez_InR, bez_UnInL, bez_UnInR, bez_SampL, bez_SampR,
        bez_cycle, bez_total
    };
    double bez[bez_total] {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CansAW)
};

} // namespace conduit::airwindows
