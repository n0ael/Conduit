/* ========================================
 *  ToneSlant - ToneSlant.cpp
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/ToneSlant —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter
 *  (entspricht dem processDoubleReplacing-Pfad des Originals).
 * ======================================== */

#include "DSP/Airwindows/Plugins/ToneSlant.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "voicing", "Voicing", 0.0f },
        { "highs",   "Highs",   0.0f },
    };
}

ToneSlant::ToneSlant() noexcept
    : AirwindowsPlugin ("toneslant", "ToneSlant", kParameters)
{
}

void ToneSlant::resetState() noexcept
{
    bL.fill (0.0);
    bR.fill (0.0);
    f.fill (0.0);
}

void ToneSlant::processStereo (const float* in1, const float* in2,
                               float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);
    const float B = param (kParamB);

    double overallscale = (A * 99.0) + 1.0;
    double applySlant = (B * 2.0) - 1.0;

    f[0] = 1.0 / overallscale;
    // count to f(gain) which will be 0. f(0) is x1
    for (int count = 1; count < 102; ++count)
    {
        if (count <= overallscale)
        {
            f[(size_t) count] = (1.0 - (count / overallscale)) / overallscale;
            // recalc the filter and don't change the buffer it'll apply to
        }
        else
        {
            bL[(size_t) count] = 0.0; // blank the unused buffer so when we return to it, no pops
            bR[(size_t) count] = 0.0; // blank the unused buffer so when we return to it, no pops
        }
    }

    while (--sampleFrames >= 0)
    {
        for (int count = (int) overallscale; count >= 0; --count)
        {
            bL[(size_t) (count + 1)] = bL[(size_t) count];
            bR[(size_t) (count + 1)] = bR[(size_t) count];
        }

        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;

        bL[0] = inputSampleL;
        bR[0] = inputSampleR;
        double accumulatorSampleL = inputSampleL;
        double accumulatorSampleR = inputSampleR;

        accumulatorSampleL *= f[0];
        accumulatorSampleR *= f[0];

        for (int count = 1; count < overallscale; ++count)
        {
            accumulatorSampleL += (bL[(size_t) count] * f[(size_t) count]);
            accumulatorSampleR += (bR[(size_t) count] * f[(size_t) count]);
        }

        double correctionSampleL = inputSampleL - (accumulatorSampleL * 2.0);
        double correctionSampleR = inputSampleR - (accumulatorSampleR * 2.0);
        // we're gonna apply the total effect of all these calculations as a single subtract

        inputSampleL += (correctionSampleL * applySlant);
        inputSampleR += (correctionSampleR * applySlant);
        // our one math operation on the input data coming in

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
