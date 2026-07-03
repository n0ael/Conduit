/* ========================================
 *  Mackity - Mackity.cpp
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Mackity —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter
 *  (entspricht dem processDoubleReplacing-Pfad des Originals).
 * ======================================== */

#include "DSP/Airwindows/Plugins/Mackity.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "in_trim", "In Trim", 0.1f },
        { "out_pad", "Out Pad", 1.0f },
    };
}

Mackity::Mackity() noexcept
    : AirwindowsPlugin ("mackity", "Mackity", kParameters)
{
}

void Mackity::resetState() noexcept
{
    iirSampleAL = 0.0;
    iirSampleBL = 0.0;
    iirSampleAR = 0.0;
    iirSampleBR = 0.0;
    for (int x = 0; x < 15; x++) { biquadA[x] = 0.0; biquadB[x] = 0.0; }
}

void Mackity::processStereo (const float* in1, const float* in2,
                             float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);
    const float B = param (kParamB);

    double overallscale = 1.0;
    overallscale /= 44100.0;
    overallscale *= getSampleRate();

    double inTrim = A * 10.0;
    double outPad = B;
    inTrim *= inTrim;

    double iirAmountA = 0.001860867 / overallscale;
    double iirAmountB = 0.000287496 / overallscale;

    biquadB[0] = biquadA[0] = 19160.0 / getSampleRate();
    biquadA[1] = 0.431684981684982;
    biquadB[1] = 1.1582298;

    double K = std::tan (juce::MathConstants<double>::pi * biquadA[0]); //lowpass
    double norm = 1.0 / (1.0 + K / biquadA[1] + K * K);
    biquadA[2] = K * K * norm;
    biquadA[3] = 2.0 * biquadA[2];
    biquadA[4] = biquadA[2];
    biquadA[5] = 2.0 * (K * K - 1.0) * norm;
    biquadA[6] = (1.0 - K / biquadA[1] + K * K) * norm;

    K = std::tan (juce::MathConstants<double>::pi * biquadB[0]);
    norm = 1.0 / (1.0 + K / biquadB[1] + K * K);
    biquadB[2] = K * K * norm;
    biquadB[3] = 2.0 * biquadB[2];
    biquadB[4] = biquadB[2];
    biquadB[5] = 2.0 * (K * K - 1.0) * norm;
    biquadB[6] = (1.0 - K / biquadB[1] + K * K) * norm;

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;

        if (std::fabs (iirSampleAL) < 1.18e-37) iirSampleAL = 0.0;
        iirSampleAL = (iirSampleAL * (1.0 - iirAmountA)) + (inputSampleL * iirAmountA);
        inputSampleL -= iirSampleAL;
        if (std::fabs (iirSampleAR) < 1.18e-37) iirSampleAR = 0.0;
        iirSampleAR = (iirSampleAR * (1.0 - iirAmountA)) + (inputSampleR * iirAmountA);
        inputSampleR -= iirSampleAR;

        if (inTrim != 1.0) { inputSampleL *= inTrim; inputSampleR *= inTrim; }

        double outSampleL = biquadA[2] * inputSampleL + biquadA[3] * biquadA[7] + biquadA[4] * biquadA[8] - biquadA[5] * biquadA[9] - biquadA[6] * biquadA[10];
        biquadA[8] = biquadA[7]; biquadA[7] = inputSampleL; inputSampleL = outSampleL; biquadA[10] = biquadA[9]; biquadA[9] = inputSampleL; //DF1 left

        double outSampleR = biquadA[2] * inputSampleR + biquadA[3] * biquadA[11] + biquadA[4] * biquadA[12] - biquadA[5] * biquadA[13] - biquadA[6] * biquadA[14];
        biquadA[12] = biquadA[11]; biquadA[11] = inputSampleR; inputSampleR = outSampleR; biquadA[14] = biquadA[13]; biquadA[13] = inputSampleR; //DF1 right

        if (inputSampleL > 1.0) inputSampleL = 1.0;
        if (inputSampleL < -1.0) inputSampleL = -1.0;
        inputSampleL -= std::pow (inputSampleL, 5) * 0.1768;
        if (inputSampleR > 1.0) inputSampleR = 1.0;
        if (inputSampleR < -1.0) inputSampleR = -1.0;
        inputSampleR -= std::pow (inputSampleR, 5) * 0.1768;

        outSampleL = biquadB[2] * inputSampleL + biquadB[3] * biquadB[7] + biquadB[4] * biquadB[8] - biquadB[5] * biquadB[9] - biquadB[6] * biquadB[10];
        biquadB[8] = biquadB[7]; biquadB[7] = inputSampleL; inputSampleL = outSampleL; biquadB[10] = biquadB[9]; biquadB[9] = inputSampleL; //DF1 left

        outSampleR = biquadB[2] * inputSampleR + biquadB[3] * biquadB[11] + biquadB[4] * biquadB[12] - biquadB[5] * biquadB[13] - biquadB[6] * biquadB[14];
        biquadB[12] = biquadB[11]; biquadB[11] = inputSampleR; inputSampleR = outSampleR; biquadB[14] = biquadB[13]; biquadB[13] = inputSampleR; //DF1 right

        if (std::fabs (iirSampleBL) < 1.18e-37) iirSampleBL = 0.0;
        iirSampleBL = (iirSampleBL * (1.0 - iirAmountB)) + (inputSampleL * iirAmountB);
        inputSampleL -= iirSampleBL;
        if (std::fabs (iirSampleBR) < 1.18e-37) iirSampleBR = 0.0;
        iirSampleBR = (iirSampleBR * (1.0 - iirAmountB)) + (inputSampleR * iirAmountB);
        inputSampleR -= iirSampleBR;

        if (outPad != 1.0) { inputSampleL *= outPad; inputSampleR *= outPad; }

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
