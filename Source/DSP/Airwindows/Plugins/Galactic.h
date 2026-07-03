/* ========================================
 *  Galactic - Galactic.h
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus plugins/LinuxVST/src/Galactic (github.com/airwindows/airwindows) —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Alle Delay-Puffer sind fest große Klassenmember (kein new/malloc im
 *  Verarbeitungspfad, CLAUDE.md 3.1) — exakt wie im Original.
 *  Original deklariert zusätzlich vibML/vibMR/depthM/thunderL/thunderR als
 *  Member — im Original-Prozesspfad nie gelesen/geschrieben (totes
 *  Altbestand-Feld einer früheren Variante), hier bewusst weggelassen
 *  (sonst -Wunused-private-field unter Clang/-Werror).
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** Galactic — Super-Reverb für Pads/Ambient (Replace/Brightness/Detune/
    Bigness/Dry-Wet), mit Vibrato-Predelay. */
class Galactic final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Replace
        kParamB = 1, // Brightness
        kParamC = 2, // Detune
        kParamD = 3, // Bigness
        kParamE = 4, // Dry/Wet
        kNumParameters = 5
    };

    Galactic() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    double iirAL = 0.0, iirAR = 0.0;
    double iirBL = 0.0, iirBR = 0.0;

    double aIL[6480] {}, aIR[6480] {};
    double aJL[3660] {}, aJR[3660] {};
    double aKL[1720] {}, aKR[1720] {};
    double aLL[680] {},  aLR[680] {};

    double aAL[9700] {}, aAR[9700] {};
    double aBL[6000] {}, aBR[6000] {};
    double aCL[2320] {}, aCR[2320] {};
    double aDL[940] {},  aDR[940] {};

    double aEL[15220] {}, aER[15220] {};
    double aFL[8460] {},  aFR[8460] {};
    double aGL[4540] {},  aGR[4540] {};
    double aHL[3200] {},  aHR[3200] {};

    double aML[3111] {}, aMR[3111] {};
    double oldfpd = 429496.7295;

    double feedbackAL = 0.0, feedbackAR = 0.0;
    double feedbackBL = 0.0, feedbackBR = 0.0;
    double feedbackCL = 0.0, feedbackCR = 0.0;
    double feedbackDL = 0.0, feedbackDR = 0.0;

    double lastRefL[7] {};
    double lastRefR[7] {};

    int countA = 1, delayA = 0;
    int countB = 1, delayB = 0;
    int countC = 1, delayC = 0;
    int countD = 1, delayD = 0;
    int countE = 1, delayE = 0;
    int countF = 1, delayF = 0;
    int countG = 1, delayG = 0;
    int countH = 1, delayH = 0;
    int countI = 1, delayI = 0;
    int countJ = 1, delayJ = 0;
    int countK = 1, delayK = 0;
    int countL = 1, delayL = 0;
    int countM = 1, delayM = 0;
    int cycle = 0;

    double vibM = 3.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Galactic)
};

} // namespace conduit::airwindows
