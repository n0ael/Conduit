/* ========================================
 *  Flutter2 - Flutter2.cpp
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Flutter2 —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter
 *  (entspricht dem processDoubleReplacing-Pfad des Originals).
 * ======================================== */

#include "DSP/Airwindows/Plugins/Flutter2.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "flutter", "Flutter", 0.5f },
        { "speed",   "Speed",   0.5f },
        { "dry_wet", "Dry/Wet", 1.0f },
    };
}

Flutter2::Flutter2() noexcept
    : AirwindowsPlugin ("flutter2", "Flutter2", kParameters)
{
}

void Flutter2::resetState() noexcept
{
    for (auto& v : dL) v = 0.0;
    for (auto& v : dR) v = 0.0;
    sweepL = juce::MathConstants<double>::pi;
    sweepR = juce::MathConstants<double>::pi;
    nextmaxL = 0.5;
    nextmaxR = 0.5;
    gcount = 0;
}

void Flutter2::processStereo (const float* in1, const float* in2,
                              float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);
    const float B = param (kParamB);
    const float C = param (kParamC);

    double overallscale = 1.0;
    overallscale /= 44100.0;
    overallscale *= getSampleRate();

    double flutDepth = std::pow (A, 4) * overallscale * 90;
    if (flutDepth > 498.0) flutDepth = 498.0;
    double flutFrequency = (0.02 * std::pow (B, 3)) / overallscale;
    double wet = C;

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;
        double drySampleL = inputSampleL;
        double drySampleR = inputSampleR;

        //begin Flutter
        if (gcount < 0 || gcount > 999) gcount = 999;

        dL[(size_t) gcount] = inputSampleL;
        int count = gcount;
        double offset = flutDepth + (flutDepth * std::sin (sweepL));
        sweepL += nextmaxL * flutFrequency;
        if (sweepL > (juce::MathConstants<double>::pi * 2.0))
        {
            sweepL -= juce::MathConstants<double>::pi * 2.0;
            nextmaxL = 0.24 + (fpdL / (double) UINT32_MAX * 0.74);
        }
        count += (int) std::floor (offset);
        inputSampleL = (dL[(size_t) (count - ((count > 999) ? 1000 : 0))] * (1 - (offset - std::floor (offset))));
        inputSampleL += (dL[(size_t) (count + 1 - ((count + 1 > 999) ? 1000 : 0))] * (offset - std::floor (offset)));

        dR[(size_t) gcount] = inputSampleR;
        count = gcount;
        offset = flutDepth + (flutDepth * std::sin (sweepR));
        sweepR += nextmaxR * flutFrequency;
        if (sweepR > (juce::MathConstants<double>::pi * 2.0))
        {
            sweepR -= juce::MathConstants<double>::pi * 2.0;
            nextmaxR = 0.24 + (fpdR / (double) UINT32_MAX * 0.74);
        }
        count += (int) std::floor (offset);
        inputSampleR = (dR[(size_t) (count - ((count > 999) ? 1000 : 0))] * (1 - (offset - std::floor (offset))));
        inputSampleR += (dR[(size_t) (count + 1 - ((count + 1 > 999) ? 1000 : 0))] * (offset - std::floor (offset)));

        gcount--;
        //end Flutter

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
