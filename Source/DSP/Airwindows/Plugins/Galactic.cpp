/* ========================================
 *  Galactic - GalacticProc.cpp
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus plugins/LinuxVST/src/Galactic (github.com/airwindows/airwindows) —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#include "DSP/Airwindows/Plugins/Galactic.h"

#include <cmath>
#include <cstdint>
#include <algorithm>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "replace",    "Replace",    0.5f },
        { "brightness", "Brightness", 0.5f },
        { "detune",     "Detune",     0.5f },
        { "bigness",    "Bigness",    1.0f },
        { "dry_wet",    "Dry/Wet",    1.0f },
    };
}

Galactic::Galactic() noexcept
    : AirwindowsPlugin ("galactic", "Galactic", kParameters)
{
}

void Galactic::resetState() noexcept
{
    iirAL = 0.0; iirAR = 0.0;
    iirBL = 0.0; iirBR = 0.0;

    std::fill (std::begin (aIL), std::end (aIL), 0.0); std::fill (std::begin (aIR), std::end (aIR), 0.0);
    std::fill (std::begin (aJL), std::end (aJL), 0.0); std::fill (std::begin (aJR), std::end (aJR), 0.0);
    std::fill (std::begin (aKL), std::end (aKL), 0.0); std::fill (std::begin (aKR), std::end (aKR), 0.0);
    std::fill (std::begin (aLL), std::end (aLL), 0.0); std::fill (std::begin (aLR), std::end (aLR), 0.0);

    std::fill (std::begin (aAL), std::end (aAL), 0.0); std::fill (std::begin (aAR), std::end (aAR), 0.0);
    std::fill (std::begin (aBL), std::end (aBL), 0.0); std::fill (std::begin (aBR), std::end (aBR), 0.0);
    std::fill (std::begin (aCL), std::end (aCL), 0.0); std::fill (std::begin (aCR), std::end (aCR), 0.0);
    std::fill (std::begin (aDL), std::end (aDL), 0.0); std::fill (std::begin (aDR), std::end (aDR), 0.0);

    std::fill (std::begin (aEL), std::end (aEL), 0.0); std::fill (std::begin (aER), std::end (aER), 0.0);
    std::fill (std::begin (aFL), std::end (aFL), 0.0); std::fill (std::begin (aFR), std::end (aFR), 0.0);
    std::fill (std::begin (aGL), std::end (aGL), 0.0); std::fill (std::begin (aGR), std::end (aGR), 0.0);
    std::fill (std::begin (aHL), std::end (aHL), 0.0); std::fill (std::begin (aHR), std::end (aHR), 0.0);

    std::fill (std::begin (aML), std::end (aML), 0.0); std::fill (std::begin (aMR), std::end (aMR), 0.0);

    feedbackAL = 0.0; feedbackAR = 0.0;
    feedbackBL = 0.0; feedbackBR = 0.0;
    feedbackCL = 0.0; feedbackCR = 0.0;
    feedbackDL = 0.0; feedbackDR = 0.0;

    std::fill (std::begin (lastRefL), std::end (lastRefL), 0.0);
    std::fill (std::begin (lastRefR), std::end (lastRefR), 0.0);

    countI = 1; countJ = 1; countK = 1; countL = 1;
    countA = 1; countB = 1; countC = 1; countD = 1;
    countE = 1; countF = 1; countG = 1; countH = 1; countM = 1;
    cycle = 0;

    vibM = 3.0;
    oldfpd = 429496.7295;
}

void Galactic::processStereo (const float* in1, const float* in2,
                              float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);
    const float B = param (kParamB);
    const float C = param (kParamC);
    const float D = param (kParamD);
    const float E = param (kParamE);

    double overallscale = 1.0;
    overallscale /= 44100.0;
    overallscale *= getSampleRate();

    int cycleEnd = (int) std::floor (overallscale);
    if (cycleEnd < 1) cycleEnd = 1;
    if (cycleEnd > 4) cycleEnd = 4;
    if (cycle > cycleEnd - 1) cycle = cycleEnd - 1;

    double regen = 0.0625 + ((1.0 - A) * 0.0625);
    double attenuate = (1.0 - (regen / 0.125)) * 1.333;
    double lowpass = std::pow (1.00001 - (1.0 - B), 2.0) / std::sqrt (overallscale);
    double drift = std::pow (C, 3) * 0.001;
    double size = (D * 1.77) + 0.1;
    double wet = 1.0 - (std::pow (1.0 - E, 3));

    delayI = (int) (3407.0 * size);
    delayJ = (int) (1823.0 * size);
    delayK = (int) (859.0 * size);
    delayL = (int) (331.0 * size);
    delayA = (int) (4801.0 * size);
    delayB = (int) (2909.0 * size);
    delayC = (int) (1153.0 * size);
    delayD = (int) (461.0 * size);
    delayE = (int) (7607.0 * size);
    delayF = (int) (4217.0 * size);
    delayG = (int) (2269.0 * size);
    delayH = (int) (1597.0 * size);
    delayM = 256;

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;
        double drySampleL = inputSampleL;
        double drySampleR = inputSampleR;

        vibM += (oldfpd * drift);
        if (vibM > (3.141592653589793238 * 2.0))
        {
            vibM = 0.0;
            oldfpd = 0.4294967295 + (fpdL * 0.0000000000618);
        }

        aML[countM] = inputSampleL * attenuate;
        aMR[countM] = inputSampleR * attenuate;
        countM++; if (countM < 0 || countM > delayM) countM = 0;

        double offsetML = (std::sin (vibM) + 1.0) * 127;
        double offsetMR = (std::sin (vibM + (3.141592653589793238 / 2.0)) + 1.0) * 127;
        int workingML = countM + (int) offsetML;
        int workingMR = countM + (int) offsetMR;
        double interpolML = (aML[workingML - ((workingML > delayM) ? delayM + 1 : 0)] * (1 - (offsetML - std::floor (offsetML))));
        interpolML += (aML[workingML + 1 - ((workingML + 1 > delayM) ? delayM + 1 : 0)] * ((offsetML - std::floor (offsetML))));
        double interpolMR = (aMR[workingMR - ((workingMR > delayM) ? delayM + 1 : 0)] * (1 - (offsetMR - std::floor (offsetMR))));
        interpolMR += (aMR[workingMR + 1 - ((workingMR + 1 > delayM) ? delayM + 1 : 0)] * ((offsetMR - std::floor (offsetMR))));
        inputSampleL = interpolML;
        inputSampleR = interpolMR;
        // Predelay mit Vibrato (Geschwindigkeit + Tiefe wie in MatrixVerb)

        iirAL = (iirAL * (1.0 - lowpass)) + (inputSampleL * lowpass); inputSampleL = iirAL;
        iirAR = (iirAR * (1.0 - lowpass)) + (inputSampleR * lowpass); inputSampleR = iirAR;

        cycle++;
        if (cycle == cycleEnd)
        {
            aIL[countI] = inputSampleL + (feedbackAR * regen);
            aJL[countJ] = inputSampleL + (feedbackBR * regen);
            aKL[countK] = inputSampleL + (feedbackCR * regen);
            aLL[countL] = inputSampleL + (feedbackDR * regen);
            aIR[countI] = inputSampleR + (feedbackAL * regen);
            aJR[countJ] = inputSampleR + (feedbackBL * regen);
            aKR[countK] = inputSampleR + (feedbackCL * regen);
            aLR[countL] = inputSampleR + (feedbackDL * regen);

            countI++; if (countI < 0 || countI > delayI) countI = 0;
            countJ++; if (countJ < 0 || countJ > delayJ) countJ = 0;
            countK++; if (countK < 0 || countK > delayK) countK = 0;
            countL++; if (countL < 0 || countL > delayL) countL = 0;

            double outIL = aIL[countI - ((countI > delayI) ? delayI + 1 : 0)];
            double outJL = aJL[countJ - ((countJ > delayJ) ? delayJ + 1 : 0)];
            double outKL = aKL[countK - ((countK > delayK) ? delayK + 1 : 0)];
            double outLL = aLL[countL - ((countL > delayL) ? delayL + 1 : 0)];
            double outIR = aIR[countI - ((countI > delayI) ? delayI + 1 : 0)];
            double outJR = aJR[countJ - ((countJ > delayJ) ? delayJ + 1 : 0)];
            double outKR = aKR[countK - ((countK > delayK) ? delayK + 1 : 0)];
            double outLR = aLR[countL - ((countL > delayL) ? delayL + 1 : 0)];

            aAL[countA] = (outIL - (outJL + outKL + outLL));
            aBL[countB] = (outJL - (outIL + outKL + outLL));
            aCL[countC] = (outKL - (outIL + outJL + outLL));
            aDL[countD] = (outLL - (outIL + outJL + outKL));
            aAR[countA] = (outIR - (outJR + outKR + outLR));
            aBR[countB] = (outJR - (outIR + outKR + outLR));
            aCR[countC] = (outKR - (outIR + outJR + outLR));
            aDR[countD] = (outLR - (outIR + outJR + outKR));

            countA++; if (countA < 0 || countA > delayA) countA = 0;
            countB++; if (countB < 0 || countB > delayB) countB = 0;
            countC++; if (countC < 0 || countC > delayC) countC = 0;
            countD++; if (countD < 0 || countD > delayD) countD = 0;

            double outAL = aAL[countA - ((countA > delayA) ? delayA + 1 : 0)];
            double outBL = aBL[countB - ((countB > delayB) ? delayB + 1 : 0)];
            double outCL = aCL[countC - ((countC > delayC) ? delayC + 1 : 0)];
            double outDL = aDL[countD - ((countD > delayD) ? delayD + 1 : 0)];
            double outAR = aAR[countA - ((countA > delayA) ? delayA + 1 : 0)];
            double outBR = aBR[countB - ((countB > delayB) ? delayB + 1 : 0)];
            double outCR = aCR[countC - ((countC > delayC) ? delayC + 1 : 0)];
            double outDR = aDR[countD - ((countD > delayD) ? delayD + 1 : 0)];

            aEL[countE] = (outAL - (outBL + outCL + outDL));
            aFL[countF] = (outBL - (outAL + outCL + outDL));
            aGL[countG] = (outCL - (outAL + outBL + outDL));
            aHL[countH] = (outDL - (outAL + outBL + outCL));
            aER[countE] = (outAR - (outBR + outCR + outDR));
            aFR[countF] = (outBR - (outAR + outCR + outDR));
            aGR[countG] = (outCR - (outAR + outBR + outDR));
            aHR[countH] = (outDR - (outAR + outBR + outCR));

            countE++; if (countE < 0 || countE > delayE) countE = 0;
            countF++; if (countF < 0 || countF > delayF) countF = 0;
            countG++; if (countG < 0 || countG > delayG) countG = 0;
            countH++; if (countH < 0 || countH > delayH) countH = 0;

            double outEL = aEL[countE - ((countE > delayE) ? delayE + 1 : 0)];
            double outFL = aFL[countF - ((countF > delayF) ? delayF + 1 : 0)];
            double outGL = aGL[countG - ((countG > delayG) ? delayG + 1 : 0)];
            double outHL = aHL[countH - ((countH > delayH) ? delayH + 1 : 0)];
            double outER = aER[countE - ((countE > delayE) ? delayE + 1 : 0)];
            double outFR = aFR[countF - ((countF > delayF) ? delayF + 1 : 0)];
            double outGR = aGR[countG - ((countG > delayG) ? delayG + 1 : 0)];
            double outHR = aHR[countH - ((countH > delayH) ? delayH + 1 : 0)];

            feedbackAL = (outEL - (outFL + outGL + outHL));
            feedbackBL = (outFL - (outEL + outGL + outHL));
            feedbackCL = (outGL - (outEL + outFL + outHL));
            feedbackDL = (outHL - (outEL + outFL + outGL));
            feedbackAR = (outER - (outFR + outGR + outHR));
            feedbackBR = (outFR - (outER + outGR + outHR));
            feedbackCR = (outGR - (outER + outFR + outHR));
            feedbackDR = (outHR - (outER + outFR + outGR));

            inputSampleL = (outEL + outFL + outGL + outHL) / 8.0;
            inputSampleR = (outER + outFR + outGR + outHR) / 8.0;

            if (cycleEnd == 4)
            {
                lastRefL[0] = lastRefL[4];
                lastRefL[2] = (lastRefL[0] + inputSampleL) / 2;
                lastRefL[1] = (lastRefL[0] + lastRefL[2]) / 2;
                lastRefL[3] = (lastRefL[2] + inputSampleL) / 2;
                lastRefL[4] = inputSampleL;
                lastRefR[0] = lastRefR[4];
                lastRefR[2] = (lastRefR[0] + inputSampleR) / 2;
                lastRefR[1] = (lastRefR[0] + lastRefR[2]) / 2;
                lastRefR[3] = (lastRefR[2] + inputSampleR) / 2;
                lastRefR[4] = inputSampleR;
            }
            if (cycleEnd == 3)
            {
                lastRefL[0] = lastRefL[3];
                lastRefL[2] = (lastRefL[0] + lastRefL[0] + inputSampleL) / 3;
                lastRefL[1] = (lastRefL[0] + inputSampleL + inputSampleL) / 3;
                lastRefL[3] = inputSampleL;
                lastRefR[0] = lastRefR[3];
                lastRefR[2] = (lastRefR[0] + lastRefR[0] + inputSampleR) / 3;
                lastRefR[1] = (lastRefR[0] + inputSampleR + inputSampleR) / 3;
                lastRefR[3] = inputSampleR;
            }
            if (cycleEnd == 2)
            {
                lastRefL[0] = lastRefL[2];
                lastRefL[1] = (lastRefL[0] + inputSampleL) / 2;
                lastRefL[2] = inputSampleL;
                lastRefR[0] = lastRefR[2];
                lastRefR[1] = (lastRefR[0] + inputSampleR) / 2;
                lastRefR[2] = inputSampleR;
            }
            if (cycleEnd == 1)
            {
                lastRefL[0] = inputSampleL;
                lastRefR[0] = inputSampleR;
            }
            cycle = 0;
            inputSampleL = lastRefL[cycle];
            inputSampleR = lastRefR[cycle];
        }
        else
        {
            inputSampleL = lastRefL[cycle];
            inputSampleR = lastRefR[cycle];
        }

        iirBL = (iirBL * (1.0 - lowpass)) + (inputSampleL * lowpass); inputSampleL = iirBL;
        iirBR = (iirBR * (1.0 - lowpass)) + (inputSampleR * lowpass); inputSampleR = iirBR;

        if (wet < 1.0)
        {
            inputSampleL = (inputSampleL * wet) + (drySampleL * (1.0 - wet));
            inputSampleR = (inputSampleR * wet) + (drySampleR * (1.0 - wet));
        }

        if (ditherOn())
        {
            int expon;
            std::frexp ((float) inputSampleL, &expon);
            fpdL ^= fpdL << 13; fpdL ^= fpdL >> 17; fpdL ^= fpdL << 5;
            inputSampleL += (double) ((double (fpdL) - std::uint32_t (0x7fffffff)) * 5.5e-36l * std::pow (2, expon + 62));
            std::frexp ((float) inputSampleR, &expon);
            fpdR ^= fpdR << 13; fpdR ^= fpdR >> 17; fpdR ^= fpdR << 5;
            inputSampleR += (double) ((double (fpdR) - std::uint32_t (0x7fffffff)) * 5.5e-36l * std::pow (2, expon + 62));
        }
        else
        {
            fpdL ^= fpdL << 13; fpdL ^= fpdL >> 17; fpdL ^= fpdL << 5;
            fpdR ^= fpdR << 13; fpdR ^= fpdR >> 17; fpdR ^= fpdR << 5;
        }

        *out1 = (float) inputSampleL;
        *out2 = (float) inputSampleR;

        ++in1;
        ++in2;
        ++out1;
        ++out2;
    }
}

} // namespace conduit::airwindows
