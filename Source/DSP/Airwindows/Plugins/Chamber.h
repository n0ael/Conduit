/* ========================================
 *  Chamber - Chamber.h
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus plugins/LinuxVST/src/Chamber (github.com/airwindows/airwindows) —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Alle Delay-Puffer sind fest große Klassenmember (kein new/malloc im
 *  Verarbeitungspfad, CLAUDE.md 3.1) — exakt wie im Original.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** Chamber — Fibonacci/Golden-Ratio-Feedback-Reverb (Bigness/Longness/
    Liteness/Darkness/Wetness). */
class Chamber final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Bigness
        kParamB = 1, // Longness
        kParamC = 2, // Liteness
        kParamD = 3, // Darkness
        kParamE = 4, // Wetness
        kNumParameters = 5
    };

    Chamber() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    double iirAL = 0.0, iirAR = 0.0;
    double iirBL = 0.0, iirBR = 0.0;
    double iirCL = 0.0, iirCR = 0.0;

    double aEL[20000] {}, aER[20000] {};
    double aFL[12361] {}, aFR[12361] {};
    double aGL[7640] {},  aGR[7640] {};
    double aHL[4722] {},  aHR[4722] {};
    double aAL[2916] {},  aAR[2916] {};
    double aBL[1804] {},  aBR[1804] {};
    double aCL[1115] {},  aCR[1115] {};
    double aDL[689] {},   aDR[689] {};
    double aIL[426] {},   aIR[426] {};
    double aJL[264] {},   aJR[264] {};
    double aKL[163] {},   aKR[163] {};
    double aLL[101] {},   aLR[101] {};

    double feedbackAL = 0.0, feedbackAR = 0.0;
    double feedbackBL = 0.0, feedbackBR = 0.0;
    double feedbackCL = 0.0, feedbackCR = 0.0;
    double feedbackDL = 0.0, feedbackDR = 0.0;
    double previousAL = 0.0, previousAR = 0.0;
    double previousBL = 0.0, previousBR = 0.0;
    double previousCL = 0.0, previousCR = 0.0;
    double previousDL = 0.0, previousDR = 0.0;

    double lastRefL[10] {};
    double lastRefR[10] {};

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
    int cycle = 0;   // Kanal-übergreifend geteilt (Original-Kommentar), nicht dupliziert

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Chamber)
};

} // namespace conduit::airwindows
