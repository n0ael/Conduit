/* ========================================
 *  Chamber - ChamberProc.cpp
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus plugins/LinuxVST/src/Chamber (github.com/airwindows/airwindows) —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#include "DSP/Airwindows/Plugins/Chamber.h"

#include <cmath>
#include <cstdint>
#include <algorithm>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "bigness",  "Bigness",  0.35f },
        { "longness", "Longness", 0.35f },
        { "liteness", "Liteness", 0.35f },
        { "darkness", "Darkness", 0.35f },
        { "wetness",  "Wetness",  0.35f },
    };
}

Chamber::Chamber() noexcept
    : AirwindowsPlugin ("chamber", "Chamber", kParameters)
{
}

void Chamber::resetState() noexcept
{
    iirAL = 0.0; iirAR = 0.0;
    iirBL = 0.0; iirBR = 0.0;
    iirCL = 0.0; iirCR = 0.0;

    std::fill (std::begin (aEL), std::end (aEL), 0.0); std::fill (std::begin (aER), std::end (aER), 0.0);
    std::fill (std::begin (aFL), std::end (aFL), 0.0); std::fill (std::begin (aFR), std::end (aFR), 0.0);
    std::fill (std::begin (aGL), std::end (aGL), 0.0); std::fill (std::begin (aGR), std::end (aGR), 0.0);
    std::fill (std::begin (aHL), std::end (aHL), 0.0); std::fill (std::begin (aHR), std::end (aHR), 0.0);
    std::fill (std::begin (aAL), std::end (aAL), 0.0); std::fill (std::begin (aAR), std::end (aAR), 0.0);
    std::fill (std::begin (aBL), std::end (aBL), 0.0); std::fill (std::begin (aBR), std::end (aBR), 0.0);
    std::fill (std::begin (aCL), std::end (aCL), 0.0); std::fill (std::begin (aCR), std::end (aCR), 0.0);
    std::fill (std::begin (aDL), std::end (aDL), 0.0); std::fill (std::begin (aDR), std::end (aDR), 0.0);
    std::fill (std::begin (aIL), std::end (aIL), 0.0); std::fill (std::begin (aIR), std::end (aIR), 0.0);
    std::fill (std::begin (aJL), std::end (aJL), 0.0); std::fill (std::begin (aJR), std::end (aJR), 0.0);
    std::fill (std::begin (aKL), std::end (aKL), 0.0); std::fill (std::begin (aKR), std::end (aKR), 0.0);
    std::fill (std::begin (aLL), std::end (aLL), 0.0); std::fill (std::begin (aLR), std::end (aLR), 0.0);

    feedbackAL = 0.0; feedbackAR = 0.0;
    feedbackBL = 0.0; feedbackBR = 0.0;
    feedbackCL = 0.0; feedbackCR = 0.0;
    feedbackDL = 0.0; feedbackDR = 0.0;
    previousAL = 0.0; previousAR = 0.0;
    previousBL = 0.0; previousBR = 0.0;
    previousCL = 0.0; previousCR = 0.0;
    previousDL = 0.0; previousDR = 0.0;

    std::fill (std::begin (lastRefL), std::end (lastRefL), 0.0);
    std::fill (std::begin (lastRefR), std::end (lastRefR), 0.0);

    countI = 1; countJ = 1; countK = 1; countL = 1;
    countA = 1; countB = 1; countC = 1; countD = 1;
    countE = 1; countF = 1; countG = 1; countH = 1;
    cycle = 0;
}

void Chamber::processStereo (const float* in1, const float* in2,
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
    // 2 für 88.1/96k, 3 für ungewöhnliche Raten, 4 für 176/192k
    if (cycle > cycleEnd - 1) cycle = cycleEnd - 1;

    double size = (std::pow (A, 2) * 0.9) + 0.1;
    double regen = (1.0 - (std::pow (1.0 - B, 6))) * 0.123;
    double highpass = (std::pow (C, 2.0)) / std::sqrt (overallscale);
    double lowpass = (1.0 - std::pow (D, 2.0)) / std::sqrt (overallscale);
    double interpolate = size * 0.381966011250105;
    double wet = E * 2.0;
    double dry = 2.0 - wet;
    if (wet > 1.0) wet = 1.0;
    if (wet < 0.0) wet = 0.0;
    if (dry > 1.0) dry = 1.0;
    if (dry < 0.0) dry = 0.0;
    // Submix-Verb: 50% voll trocken UND voll nass, nicht überblendet —
    // so bleibt beim Hinzuregeln der andere Mix-Anteil unverändert.

    delayE = (int) (19900 * size);
    delayF = (int) (delayE * 0.618033988749894848204586);
    delayG = (int) (delayF * 0.618033988749894848204586);
    delayH = (int) (delayG * 0.618033988749894848204586);
    delayA = (int) (delayH * 0.618033988749894848204586);
    delayB = (int) (delayA * 0.618033988749894848204586);
    delayC = (int) (delayB * 0.618033988749894848204586);
    delayD = (int) (delayC * 0.618033988749894848204586);
    delayI = (int) (delayD * 0.618033988749894848204586);
    delayJ = (int) (delayI * 0.618033988749894848204586);
    delayK = (int) (delayJ * 0.618033988749894848204586);
    delayL = (int) (delayK * 0.618033988749894848204586);
    // Golden-Ratio-Kette (Fibonacci-Verhältnis) — aus Slapback wird bei
    // anhaltender Rückkopplung ein unendlich sustainender Reverb-Tail.

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;
        double drySampleL = inputSampleL;
        double drySampleR = inputSampleR;

        iirCL = (iirCL * (1.0 - highpass)) + (inputSampleL * highpass); inputSampleL -= iirCL;
        iirCR = (iirCR * (1.0 - highpass)) + (inputSampleR * highpass); inputSampleR -= iirCR;
        // initialer Hochpass

        iirAL = (iirAL * (1.0 - lowpass)) + (inputSampleL * lowpass); inputSampleL = iirAL;
        iirAR = (iirAR * (1.0 - lowpass)) + (inputSampleR * lowpass); inputSampleR = iirAR;
        // initialer Filter

        cycle++;
        if (cycle == cycleEnd) // Endpunkt erreicht -> ein Reverb-Sample
        {
            feedbackAL = (feedbackAL * (1.0 - interpolate)) + (previousAL * interpolate); previousAL = feedbackAL;
            feedbackBL = (feedbackBL * (1.0 - interpolate)) + (previousBL * interpolate); previousBL = feedbackBL;
            feedbackCL = (feedbackCL * (1.0 - interpolate)) + (previousCL * interpolate); previousCL = feedbackCL;
            feedbackDL = (feedbackDL * (1.0 - interpolate)) + (previousDL * interpolate); previousDL = feedbackDL;
            feedbackAR = (feedbackAR * (1.0 - interpolate)) + (previousAR * interpolate); previousAR = feedbackAR;
            feedbackBR = (feedbackBR * (1.0 - interpolate)) + (previousBR * interpolate); previousBR = feedbackBR;
            feedbackCR = (feedbackCR * (1.0 - interpolate)) + (previousCR * interpolate); previousCR = feedbackCR;
            feedbackDR = (feedbackDR * (1.0 - interpolate)) + (previousDR * interpolate); previousDR = feedbackDR;

            aIL[countI] = inputSampleL + (feedbackAL * regen);
            aJL[countJ] = inputSampleL + (feedbackBL * regen);
            aKL[countK] = inputSampleL + (feedbackCL * regen);
            aLL[countL] = inputSampleL + (feedbackDL * regen);
            aIR[countI] = inputSampleR + (feedbackAR * regen);
            aJR[countJ] = inputSampleR + (feedbackBR * regen);
            aKR[countK] = inputSampleR + (feedbackCR * regen);
            aLR[countL] = inputSampleR + (feedbackDR * regen);

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
            // erster Block: vier Ausgänge

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
            // zweiter Block: vier weitere Ausgänge

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
            // dritter Block: finale Ausgänge

            feedbackAL = (outEL - (outFL + outGL + outHL));
            feedbackBL = (outFL - (outEL + outGL + outHL));
            feedbackCL = (outGL - (outEL + outFL + outHL));
            feedbackDL = (outHL - (outEL + outFL + outGL));
            feedbackAR = (outER - (outFR + outGR + outHR));
            feedbackBR = (outFR - (outER + outGR + outHR));
            feedbackCR = (outGR - (outER + outFR + outHR));
            feedbackDR = (outHR - (outER + outFR + outGR));
            // Rückkopplung in den Eingang

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

        switch (cycleEnd) // Mehrpol-Mittelung über lastRef[]
        {
            case 4:
                lastRefL[8] = inputSampleL; inputSampleL = (inputSampleL + lastRefL[7]) * 0.5;
                lastRefL[7] = lastRefL[8];
                lastRefR[8] = inputSampleR; inputSampleR = (inputSampleR + lastRefR[7]) * 0.5;
                lastRefR[7] = lastRefR[8];
                [[fallthrough]];
            case 3:
                lastRefL[8] = inputSampleL; inputSampleL = (inputSampleL + lastRefL[6]) * 0.5;
                lastRefL[6] = lastRefL[8];
                lastRefR[8] = inputSampleR; inputSampleR = (inputSampleR + lastRefR[6]) * 0.5;
                lastRefR[6] = lastRefR[8];
                [[fallthrough]];
            case 2:
                lastRefL[8] = inputSampleL; inputSampleL = (inputSampleL + lastRefL[5]) * 0.5;
                lastRefL[5] = lastRefL[8];
                lastRefR[8] = inputSampleR; inputSampleR = (inputSampleR + lastRefR[5]) * 0.5;
                lastRefR[5] = lastRefR[8];
                [[fallthrough]];
            case 1:
                break;
        }

        iirBL = (iirBL * (1.0 - lowpass)) + (inputSampleL * lowpass); inputSampleL = iirBL;
        iirBR = (iirBR * (1.0 - lowpass)) + (inputSampleR * lowpass); inputSampleR = iirBR;
        // Endfilter

        if (wet < 1.0) { inputSampleL *= wet; inputSampleR *= wet; }
        if (dry < 1.0) { drySampleL *= dry; drySampleR *= dry; }
        inputSampleL += drySampleL;
        inputSampleR += drySampleR;

        // 32-Bit-Stereo-Float-Dither
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
