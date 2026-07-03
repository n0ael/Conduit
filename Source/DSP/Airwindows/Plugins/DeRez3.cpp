/* ========================================
 *  DeRez3 - DeRez3.cpp
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/DeRez3 —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter.
 * ======================================== */

#include "DSP/Airwindows/Plugins/DeRez3.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "rate",    "Rate",    1.0f },
        { "rez",     "Rez",     1.0f },
        { "dry_wet", "Dry/Wet", 1.0f },
    };
}

DeRez3::DeRez3() noexcept
    : AirwindowsPlugin ("derez3", "DeRez3", kParameters)
{
}

void DeRez3::resetState() noexcept
{
    for (double& v : bez) v = 0.0;
    bez[bez_cycle] = 1.0;

    rezA = 1.0; rezB = 1.0;
    bitA = 1.0; bitB = 1.0;
    wetA = 1.0; wetB = 1.0;
}

void DeRez3::processStereo (const float* in1, const float* in2,
                           float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);
    const float B = param (kParamB);
    const float C = param (kParamC);

    const int inFramesToProcess = sampleFrames;
    double overallscale = 1.0;
    overallscale /= 44100.0;
    overallscale *= getSampleRate();

    rezA = rezB;
    rezB = std::pow (A, 3.0) / overallscale;
    bitA = bitB;
    bitB = (B * 15.0) + 1.0;
    wetA = wetB;
    wetB = C * 2.0;

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
        double bit = (bitA * temp) + (bitB * (1.0 - temp));
        double wet = (wetA * temp) + (wetB * (1.0 - temp));
        if (rez < 0.0005) rez = 0.0005;
        double bitFactor = std::pow (2.0, bit);
        double dry = 2.0 - wet;
        if (wet > 1.0) wet = 1.0;
        if (wet < 0.0) wet = 0.0;
        if (dry > 1.0) dry = 1.0;
        if (dry < 0.0) dry = 0.0;
        //this bitcrush makes 50% full dry AND full wet, not crossfaded.
        //that's so it can be on tracks without cutting back dry channel when adjusted

        inputSampleL *= bitFactor;
        inputSampleL = std::floor (inputSampleL + (0.5 / bitFactor));
        inputSampleL /= bitFactor;
        inputSampleR *= bitFactor;
        inputSampleR = std::floor (inputSampleR + (0.5 / bitFactor));
        inputSampleR /= bitFactor;

        bez[bez_cycle] += rez;
        bez[bez_SampL] += (inputSampleL * rez);
        bez[bez_SampR] += (inputSampleR * rez);
        if (bez[bez_cycle] > 1.0)
        {
            bez[bez_cycle] -= 1.0;
            bez[bez_CL] = bez[bez_BL];
            bez[bez_BL] = bez[bez_AL];
            bez[bez_AL] = inputSampleL;
            bez[bez_SampL] = 0.0;
            bez[bez_CR] = bez[bez_BR];
            bez[bez_BR] = bez[bez_AR];
            bez[bez_AR] = inputSampleR;
            bez[bez_SampR] = 0.0;
        }
        double CBL = (bez[bez_CL] * (1.0 - bez[bez_cycle])) + (bez[bez_BL] * bez[bez_cycle]);
        double CBR = (bez[bez_CR] * (1.0 - bez[bez_cycle])) + (bez[bez_BR] * bez[bez_cycle]);
        double BAL = (bez[bez_BL] * (1.0 - bez[bez_cycle])) + (bez[bez_AL] * bez[bez_cycle]);
        double BAR = (bez[bez_BR] * (1.0 - bez[bez_cycle])) + (bez[bez_AR] * bez[bez_cycle]);
        double CBAL = (bez[bez_BL] + (CBL * (1.0 - bez[bez_cycle])) + (BAL * bez[bez_cycle])) * 0.5;
        double CBAR = (bez[bez_BR] + (CBR * (1.0 - bez[bez_cycle])) + (BAR * bez[bez_cycle])) * 0.5;

        inputSampleL = (wet * CBAL) + (dry * drySampleL);
        inputSampleR = (wet * CBAR) + (dry * drySampleR);

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
