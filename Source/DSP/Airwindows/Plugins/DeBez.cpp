/* ========================================
 *  DeBez - DeBez.cpp
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/DeBez —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Original hat KEIN ditherOn()-Gate — der 32-Bit-Dither-Add läuft im
 *  Original immer; hier hinter ditherOn() (Projekt-Konvention), Xorshift
 *  läuft immer weiter.
 * ======================================== */

#include "DSP/Airwindows/Plugins/DeBez.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "de_bez",  "DeBez",   0.5f },
        { "de_rez",  "DeRez",   0.5f },
        { "inv_wet", "Inv/Wet", 1.0f },
    };
}

DeBez::DeBez() noexcept
    : AirwindowsPlugin ("debez", "DeBez", kParameters)
{
}

void DeBez::resetState() noexcept
{
    for (double& v : bezF) v = 0.0;
    bezF[bez_cycle] = 1.0;

    rezA = 0.5; rezB = 0.5;
    bitA = 0.5; bitB = 0.5;
    wetA = 1.0; wetB = 1.0;
}

void DeBez::processStereo (const float* in1, const float* in2,
                           float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);
    const float B = param (kParamB);
    const float C = param (kParamC);

    const int inFramesToProcess = sampleFrames;

    rezA = rezB; rezB = A * 2.0;
    bitA = bitB; bitB = B * 2.0;
    wetA = wetB; wetB = C * 2.0;

    bool steppedFreq = true; // Revised Bezier Undersampling
    if (rezB > 1.0)
    {
        steppedFreq = false;
        rezB = 1.0 - (rezB - 1.0);
    }
    rezB = std::fmin (std::fmax (std::pow (rezB, 3.0), 0.0005), 1.0);
    int bezFreqFraction = (int) (1.0 / rezB);
    double bezFreqTrim = (double) bezFreqFraction / (bezFreqFraction + 1.0);
    if (steppedFreq)
    {
        rezB = 1.0 / bezFreqFraction;
        bezFreqTrim = 1.0 - (rezB * bezFreqTrim);
    }
    else
    {
        bezFreqTrim = 1.0 - std::pow (rezB * 0.5, 1.0 / (rezB * 0.5));
    }

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;
        double drySampleL = inputSampleL;
        double drySampleR = inputSampleR;
        double temp = (double) sampleFrames / inFramesToProcess;
        double rez = (rezA * temp) + (rezB * (1.0 - temp));
        double bit = ((bitA * temp) + (bitB * (1.0 - temp))) - 1.0;
        double wet = ((wetA * temp) + (wetB * (1.0 - temp))) - 1.0;
        double dry = 1.0 - wet;
        if (wet > 1.0) wet = 1.0;
        if (wet < -1.0) wet = -1.0;
        if (dry > 1.0) dry = 1.0;
        if (dry < 0.0) dry = 0.0;

        bezF[bez_cycle] += rez;
        bezF[bez_SampL] += (inputSampleL * rez);
        bezF[bez_SampR] += (inputSampleR * rez);
        if (bezF[bez_cycle] > 1.0)
        {
            if (steppedFreq) bezF[bez_cycle] = 0.0;
            else bezF[bez_cycle] -= 1.0;

            inputSampleL = (bezF[bez_SampL] + bezF[bez_AvgInSampL]) * 0.5;
            bezF[bez_AvgInSampL] = bezF[bez_SampL];
            inputSampleR = (bezF[bez_SampR] + bezF[bez_AvgInSampR]) * 0.5;
            bezF[bez_AvgInSampR] = bezF[bez_SampR];

            bool crushGate = (bit < 0.0);
            bit = 1.0 - std::fabs (bit);
            bit = std::fmin (std::fmax (bit * 16.0, 0.5), 16.0);
            double bitFactor = std::pow (2.0, bit);
            inputSampleL *= bitFactor;
            inputSampleL = std::floor (inputSampleL + (crushGate ? 0.5 / bitFactor : 0.0));
            inputSampleL /= bitFactor;
            inputSampleR *= bitFactor;
            inputSampleR = std::floor (inputSampleR + (crushGate ? 0.5 / bitFactor : 0.0));
            inputSampleR /= bitFactor;
            //derez inside debez
            bezF[bez_CL] = bezF[bez_BL];
            bezF[bez_BL] = bezF[bez_AL];
            bezF[bez_AL] = inputSampleL;
            bezF[bez_SampL] = 0.0;
            bezF[bez_CR] = bezF[bez_BR];
            bezF[bez_BR] = bezF[bez_AR];
            bezF[bez_AR] = inputSampleR;
            bezF[bez_SampR] = 0.0;
        }
        double X = bezF[bez_cycle] * bezFreqTrim;
        double CBLfreq = (bezF[bez_CL] * (1.0 - X)) + (bezF[bez_BL] * X);
        double BALfreq = (bezF[bez_BL] * (1.0 - X)) + (bezF[bez_AL] * X);
        double CBALfreq = (bezF[bez_BL] + (CBLfreq * (1.0 - X)) + (BALfreq * X)) * 0.125;
        inputSampleL = CBALfreq + bezF[bez_AvgOutSampL]; bezF[bez_AvgOutSampL] = CBALfreq;

        double CBRfreq = (bezF[bez_CR] * (1.0 - X)) + (bezF[bez_BR] * X);
        double BARfreq = (bezF[bez_BR] * (1.0 - X)) + (bezF[bez_AR] * X);
        double CBARfreq = (bezF[bez_BR] + (CBRfreq * (1.0 - X)) + (BARfreq * X)) * 0.125;
        inputSampleR = CBARfreq + bezF[bez_AvgOutSampR]; bezF[bez_AvgOutSampR] = CBARfreq;

        inputSampleL = (wet * inputSampleL) + (dry * drySampleL);
        inputSampleR = (wet * inputSampleR) + (dry * drySampleR);

        //begin 32 bit stereo floating point dither
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
        //end 32 bit stereo floating point dither

        *out1 = (float) inputSampleL;
        *out2 = (float) inputSampleR;

        ++in1;
        ++in2;
        ++out1;
        ++out2;
    }
}

} // namespace conduit::airwindows
