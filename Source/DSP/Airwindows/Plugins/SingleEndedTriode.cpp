/* ========================================
 *  SingleEndedTriode - SingleEndedTriode.cpp
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/SingleEndedTriode —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter
 *  (entspricht dem processDoubleReplacing-Pfad des Originals).
 * ======================================== */

#include "DSP/Airwindows/Plugins/SingleEndedTriode.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "triode",  "Triode",  0.0f },
        { "clas_ab", "Clas AB", 0.0f },
        { "clas_b",  "Clas B",  0.0f },
        { "dry_wet", "Dry/Wet", 0.0f },
    };
}

SingleEndedTriode::SingleEndedTriode() noexcept
    : AirwindowsPlugin ("singleendedtriode", "SingleEndedTriode", kParameters)
{
    postsine = std::sin (0.5);
}

void SingleEndedTriode::resetState() noexcept
{
    postsine = std::sin (0.5);
}

void SingleEndedTriode::processStereo (const float* in1, const float* in2,
                                       float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);
    const float B = param (kParamB);
    const float C = param (kParamC);
    const float D = param (kParamD);

    double intensity = std::pow (A, 2) * 8.0;
    double triode = intensity;
    intensity += 0.001;
    double softcrossover = std::pow (B, 3) / 8.0;
    double hardcrossover = std::pow (C, 7) / 8.0;
    double wet = D;
    //removed extra dry variable

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;
        double drySampleL = inputSampleL;
        double drySampleR = inputSampleR;

        if (triode > 0.0)
        {
            inputSampleL *= intensity;
            inputSampleR *= intensity;
            inputSampleL -= 0.5;
            inputSampleR -= 0.5;

            double bridgerectifier = std::fabs (inputSampleL);
            if (bridgerectifier > 1.57079633) bridgerectifier = 1.57079633;
            bridgerectifier = std::sin (bridgerectifier);
            if (inputSampleL > 0) inputSampleL = bridgerectifier;
            else inputSampleL = -bridgerectifier;

            bridgerectifier = std::fabs (inputSampleR);
            if (bridgerectifier > 1.57079633) bridgerectifier = 1.57079633;
            bridgerectifier = std::sin (bridgerectifier);
            if (inputSampleR > 0) inputSampleR = bridgerectifier;
            else inputSampleR = -bridgerectifier;

            inputSampleL += postsine;
            inputSampleR += postsine;
            inputSampleL /= intensity;
            inputSampleR /= intensity;
        }

        if (softcrossover > 0.0)
        {
            double bridgerectifier = std::fabs (inputSampleL);
            if (bridgerectifier > 0.0) bridgerectifier -= (softcrossover * (bridgerectifier + std::sqrt (bridgerectifier)));
            if (bridgerectifier < 0.0) bridgerectifier = 0;
            if (inputSampleL > 0.0) inputSampleL = bridgerectifier;
            else inputSampleL = -bridgerectifier;

            bridgerectifier = std::fabs (inputSampleR);
            if (bridgerectifier > 0.0) bridgerectifier -= (softcrossover * (bridgerectifier + std::sqrt (bridgerectifier)));
            if (bridgerectifier < 0.0) bridgerectifier = 0;
            if (inputSampleR > 0.0) inputSampleR = bridgerectifier;
            else inputSampleR = -bridgerectifier;
        }


        if (hardcrossover > 0.0)
        {
            double bridgerectifier = std::fabs (inputSampleL);
            bridgerectifier -= hardcrossover;
            if (bridgerectifier < 0.0) bridgerectifier = 0.0;
            if (inputSampleL > 0.0) inputSampleL = bridgerectifier;
            else inputSampleL = -bridgerectifier;

            bridgerectifier = std::fabs (inputSampleR);
            bridgerectifier -= hardcrossover;
            if (bridgerectifier < 0.0) bridgerectifier = 0.0;
            if (inputSampleR > 0.0) inputSampleR = bridgerectifier;
            else inputSampleR = -bridgerectifier;
        }

        if (wet != 1.0)
        {
            inputSampleL = (inputSampleL * wet) + (drySampleL * (1.0 - wet));
            inputSampleR = (inputSampleR * wet) + (drySampleR * (1.0 - wet));
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
