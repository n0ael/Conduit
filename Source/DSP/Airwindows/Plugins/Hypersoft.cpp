/* ========================================
 *  Hypersoft - Hypersoft.cpp
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Hypersoft —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter
 *  (entspricht dem processDoubleReplacing-Pfad des Originals).
 * ======================================== */

#include "DSP/Airwindows/Plugins/Hypersoft.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "input",  "Input",  0.5f },
        { "depth",  "Depth",  0.5f },
        { "bright", "Bright", 0.5f },
        { "output", "Output", 0.5f },
    };
}

Hypersoft::Hypersoft() noexcept
    : AirwindowsPlugin ("hypersoft", "Hypersoft", kParameters)
{
}

void Hypersoft::resetState() noexcept
{
    lastSampleL = 0.0;
    lastSampleR = 0.0;
}

void Hypersoft::processStereo (const float* in1, const float* in2,
                               float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);
    const float B = param (kParamB);
    const float C = param (kParamC);
    const float D = param (kParamD);

    double inputGain = A * 2.0;
    if (inputGain > 1.0) inputGain *= inputGain; else inputGain = 1.0 - std::pow (1.0 - inputGain, 2);
    //this is the fader curve from ConsoleX with 0.5 being unity gain
    int stages = (int) (B * 12.0) + 2;
    //each stage brings in an additional layer of harmonics on the waveshaping
    double bright = (1.0 - C) * 0.15;
    //higher slews suppress these higher harmonics when they are sure to just alias
    double outputGain = D * 2.0;
    if (outputGain > 1.0) outputGain *= outputGain; else outputGain = 1.0 - std::pow (1.0 - outputGain, 2);
    outputGain *= 0.68;
    //this is the fader curve from ConsoleX, rescaled to work with Hypersoft

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;

        inputSampleL *= inputGain;
        inputSampleR *= inputGain;

        inputSampleL = std::sin (inputSampleL); inputSampleL += (std::sin (inputSampleL * 2.0) / 2.0);
        inputSampleR = std::sin (inputSampleR); inputSampleR += (std::sin (inputSampleR * 2.0) / 2.0);
        for (int count = 2; count < stages; count++)
        {
            inputSampleL += ((std::sin (inputSampleL * (double) count) / std::pow ((double) count, 3)) * std::max (0.0, 1.0 - std::fabs ((inputSampleL - lastSampleL) * bright * (double) (count * count))));
            inputSampleR += ((std::sin (inputSampleR * (double) count) / std::pow ((double) count, 3)) * std::max (0.0, 1.0 - std::fabs ((inputSampleR - lastSampleR) * bright * (double) (count * count))));
        }
        lastSampleL = inputSampleL;
        lastSampleR = inputSampleR;

        inputSampleL *= outputGain;
        inputSampleR *= outputGain;

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
