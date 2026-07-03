/* ========================================
 *  PearEQ - PearEQ.cpp
 *  Copyright (c) Airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/PearEQ —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter
 *  (entspricht dem processDoubleReplacing-Pfad des Originals).
 * ======================================== */

#include "DSP/Airwindows/Plugins/PearEQ.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "high", "High", 0.5f },
        { "hmid", "HMid", 0.5f },
        { "mid",  "Mid",  0.5f },
        { "lmid", "LMid", 0.5f },
        { "bass", "Bass", 0.5f },
        { "sub",  "Sub",  0.5f },
    };
}

PearEQ::PearEQ() noexcept
    : AirwindowsPlugin ("peareq", "PearEQ", kParameters)
{
}

void PearEQ::resetState() noexcept
{
    for (auto& v : pearA) v = 0.0;
    for (auto& v : pearB) v = 0.0;
    for (auto& v : pearC) v = 0.0;
    for (auto& v : pearD) v = 0.0;
    for (auto& v : pearE) v = 0.0;
}

void PearEQ::processStereo (const float* in1, const float* in2,
                            float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);
    const float B = param (kParamB);
    const float C = param (kParamC);
    const float D = param (kParamD);
    const float E = param (kParamE);
    const float F = param (kParamF);

    double overallscale = 1.0;
    overallscale /= 44100.0;
    overallscale *= getSampleRate();

    double topLevl = std::sqrt (A + 0.5);
    pearA[levl] = std::sqrt (B + 0.5);
    pearB[levl] = std::sqrt (C + 0.5);
    pearC[levl] = std::sqrt (D + 0.5);
    pearD[levl] = std::sqrt (E + 0.5);
    pearE[levl] = std::sqrt (F + 0.5);

    double freqFactor = std::sqrt (overallscale) + (overallscale * 0.5);
    pearA[freq] = std::pow (0.564, freqFactor + 0.85);
    pearB[freq] = std::pow (0.564, freqFactor + 1.9);
    pearC[freq] = std::pow (0.564, freqFactor + 4.1);
    pearD[freq] = std::pow (0.564, freqFactor + 6.2);
    pearE[freq] = std::pow (0.564, freqFactor + 7.7);

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;

        for (int x = 0; x < pear_max; x += 4)
        {
            //begin Pear filter stages
            pearA[figL] = inputSampleL; pearA[figR] = inputSampleR;
            pearA[slew] = ((pearA[figL] - pearA[x]) + pearA[x + 1]) * pearA[freq] * 0.5;
            pearA[x] = pearA[figL] = (pearA[freq] * pearA[figL]) + ((1.0 - pearA[freq]) * (pearA[x] + pearA[x + 1]));
            pearA[x + 1] = pearA[slew];
            pearA[slew] = ((pearA[figR] - pearA[x + 2]) + pearA[x + 3]) * pearA[freq] * 0.5;
            pearA[x + 2] = pearA[figR] = (pearA[freq] * pearA[figR]) + ((1.0 - pearA[freq]) * (pearA[x + 2] + pearA[x + 3]));
            pearA[x + 3] = pearA[slew];
            inputSampleL -= pearA[figL]; inputSampleR -= pearA[figR];

            pearB[figL] = pearA[figL]; pearB[figR] = pearA[figR];
            pearB[slew] = ((pearB[figL] - pearB[x]) + pearB[x + 1]) * pearB[freq] * 0.5;
            pearB[x] = pearB[figL] = (pearB[freq] * pearA[figL]) + ((1.0 - pearB[freq]) * (pearB[x] + pearB[x + 1]));
            pearB[x + 1] = pearB[slew];
            pearB[slew] = ((pearB[figR] - pearB[x + 2]) + pearB[x + 3]) * pearB[freq] * 0.5;
            pearB[x + 2] = pearB[figR] = (pearB[freq] * pearA[figR]) + ((1.0 - pearB[freq]) * (pearB[x + 2] + pearB[x + 3]));
            pearB[x + 3] = pearB[slew];
            pearA[figL] -= pearB[figL]; pearA[figR] -= pearB[figR];

            pearC[figL] = pearB[figL]; pearC[figR] = pearB[figR];
            pearC[slew] = ((pearC[figL] - pearC[x]) + pearC[x + 1]) * pearC[freq] * 0.5;
            pearC[x] = pearC[figL] = (pearC[freq] * pearB[figL]) + ((1.0 - pearC[freq]) * (pearC[x] + pearC[x + 1]));
            pearC[x + 1] = pearC[slew];
            pearC[slew] = ((pearC[figR] - pearC[x + 2]) + pearC[x + 3]) * pearC[freq] * 0.5;
            pearC[x + 2] = pearC[figR] = (pearC[freq] * pearB[figR]) + ((1.0 - pearC[freq]) * (pearC[x + 2] + pearC[x + 3]));
            pearC[x + 3] = pearC[slew];
            pearB[figL] -= pearC[figL]; pearB[figR] -= pearC[figR];

            pearD[figL] = pearC[figL]; pearD[figR] = pearC[figR];
            pearD[slew] = ((pearD[figL] - pearD[x]) + pearD[x + 1]) * pearD[freq] * 0.5;
            pearD[x] = pearD[figL] = (pearD[freq] * pearC[figL]) + ((1.0 - pearD[freq]) * (pearD[x] + pearD[x + 1]));
            pearD[x + 1] = pearD[slew];
            pearD[slew] = ((pearD[figR] - pearD[x + 2]) + pearD[x + 3]) * pearD[freq] * 0.5;
            pearD[x + 2] = pearD[figR] = (pearD[freq] * pearC[figR]) + ((1.0 - pearD[freq]) * (pearD[x + 2] + pearD[x + 3]));
            pearD[x + 3] = pearD[slew];
            pearC[figL] -= pearD[figL]; pearC[figR] -= pearD[figR];

            pearE[figL] = pearD[figL]; pearE[figR] = pearD[figR];
            pearE[slew] = ((pearE[figL] - pearE[x]) + pearE[x + 1]) * pearE[freq] * 0.5;
            pearE[x] = pearE[figL] = (pearE[freq] * pearD[figL]) + ((1.0 - pearE[freq]) * (pearE[x] + pearE[x + 1]));
            pearE[x + 1] = pearE[slew];
            pearE[slew] = ((pearE[figR] - pearE[x + 2]) + pearE[x + 3]) * pearE[freq] * 0.5;
            pearE[x + 2] = pearE[figR] = (pearE[freq] * pearD[figR]) + ((1.0 - pearE[freq]) * (pearE[x + 2] + pearE[x + 3]));
            pearE[x + 3] = pearE[slew];
            pearD[figL] -= pearE[figL]; pearD[figR] -= pearE[figR];

            inputSampleL *= topLevl; inputSampleR *= topLevl;
            inputSampleL += (pearA[figL] * pearA[levl]); inputSampleR += (pearA[figR] * pearA[levl]);
            inputSampleL += (pearB[figL] * pearB[levl]); inputSampleR += (pearB[figR] * pearB[levl]);
            inputSampleL += (pearC[figL] * pearC[levl]); inputSampleR += (pearC[figR] * pearC[levl]);
            inputSampleL += (pearD[figL] * pearD[levl]); inputSampleR += (pearD[figR] * pearD[levl]);
            inputSampleL += (pearE[figL] * pearE[levl]); inputSampleR += (pearE[figR] * pearE[levl]);
        }

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
