/* ========================================
 *  Cans - Cans.h
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Cans —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** Cans — Headphone-Reverb-Simulation ("StudioA".."StudioE" Raumgrößen via
    Room-Parameter), Diffuse/Damping/Crossfeed/Dry-Wet. Bezier-Undersampling
    (16 verschachtelte Delay-Linien, feste Größen je Raum-Preset). */
class Cans final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Room
        kParamB = 1, // Diffuse
        kParamC = 2, // Damping
        kParamD = 3, // Crossfd
        kParamE = 4, // Dry/Wet
        kNumParameters = 5
    };

    Cans() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    static constexpr int kShortA = 193;
    static constexpr int kShortB = 588;
    static constexpr int kShortC = 551;
    static constexpr int kShortD = 325;
    static constexpr int kShortE = 166;
    static constexpr int kShortF = 427;
    static constexpr int kShortG = 313;
    static constexpr int kShortH = 575;
    static constexpr int kShortI = 101;
    static constexpr int kShortJ = 794;
    static constexpr int kShortK = 789;
    static constexpr int kShortL = 385;
    static constexpr int kShortM = 12;
    static constexpr int kShortN = 1009;
    static constexpr int kShortO = 64;
    static constexpr int kShortP = 544;

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

    int shortA = 23, shortB = 357, shortC = 305, shortD = 186, shortE = 104, shortF = 255, shortG = 163, shortH = 147;
    int shortI = 56, shortJ = 480, shortK = 317, shortL = 107, shortM = 11, shortN = 704, shortO = 26, shortP = 543;

    int prevClearcoat = -1;

    enum BezIndex
    {
        bez_AL, bez_AR, bez_BL, bez_BR, bez_CL, bez_CR,
        bez_InL, bez_InR, bez_UnInL, bez_UnInR, bez_SampL, bez_SampR,
        bez_cycle, bez_total
    };
    double bez[bez_total] {};

    /** Setzt die 16 Delay-Line-Größen für das gegebene Raum-Preset
        (Original-`clearcoat`-Switch, identisch zur Konstruktor-/Proc-Logik). */
    void applyRoomPreset (int clearcoat) noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Cans)
};

} // namespace conduit::airwindows
