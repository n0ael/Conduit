/* ========================================
 *  Weight - Weight.cpp
 *  Copyright (c) Airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Weight —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter
 *  (entspricht dem processDoubleReplacing-Pfad des Originals).
 * ======================================== */

#include "DSP/Airwindows/Plugins/Weight.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "freq",   "Freq",   0.5f },
        { "weight", "Weight", 0.0f },
    };
}

Weight::Weight() noexcept
    : AirwindowsPlugin ("weight", "Weight", kParameters)
{
}

void Weight::resetState() noexcept
{
    previousSampleL.fill (0.0);
    previousTrendL.fill (0.0);
    previousSampleR.fill (0.0);
    previousTrendR.fill (0.0);
}

void Weight::processStereo (const float* in1, const float* in2,
                            float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);
    const float B = param (kParamB);

    double overallscale = 1.0;
    overallscale /= 44100.0;
    overallscale *= getSampleRate();

    double targetFreq = A;
    // gives us a 0-1 value like the VST will be. For the VST, start with 0-1 and
    // have the plugin display the number as 20-120.

    targetFreq = ((targetFreq + 0.53) * 0.2) / std::sqrt (overallscale);
    // must use square root of what the real scale would be, to get correct output
    double alpha = std::pow (targetFreq, 4);
    double out = B;
    double resControl = (out * 0.05) + 0.2;
    double beta = (alpha * std::pow (resControl, 2));
    alpha += ((1.0 - beta) * std::pow (targetFreq, 3)); // correct for droop in frequency

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;
        double drySampleL = inputSampleL;
        double drySampleR = inputSampleR;

        // begin Weight
        double trend;
        double forecast;
        for (int i = 0; i < 8; ++i)
        {
            trend = (beta * (inputSampleL - previousSampleL[(size_t) i]) + ((0.999 - beta) * previousTrendL[(size_t) i]));
            forecast = previousSampleL[(size_t) i] + previousTrendL[(size_t) i];
            inputSampleL = (alpha * inputSampleL) + ((0.999 - alpha) * forecast);
            previousSampleL[(size_t) i] = inputSampleL; previousTrendL[(size_t) i] = trend;

            trend = (beta * (inputSampleR - previousSampleR[(size_t) i]) + ((0.999 - beta) * previousTrendR[(size_t) i]));
            forecast = previousSampleR[(size_t) i] + previousTrendR[(size_t) i];
            inputSampleR = (alpha * inputSampleR) + ((0.999 - alpha) * forecast);
            previousSampleR[(size_t) i] = inputSampleR; previousTrendR[(size_t) i] = trend;
        }
        // inputSample is now the bass boost to be added

        inputSampleL *= out;
        inputSampleR *= out;
        inputSampleL += drySampleL;
        inputSampleR += drySampleR;

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
