/* ========================================
 *  SoftGate - SoftGate.cpp
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/SoftGate —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter
 *  (entspricht dem processDoubleReplacing-Pfad des Originals).
 * ======================================== */

#include "DSP/Airwindows/Plugins/SoftGate.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "thresh",  "Thresh",  0.5f },
        { "darken",  "Darken",  0.5f },
        { "silence", "Silence", 0.0f },
    };
}

SoftGate::SoftGate() noexcept
    : AirwindowsPlugin ("softgate", "SoftGate", kParameters)
{
}

void SoftGate::resetState() noexcept
{
    storedL[0] = storedL[1] = 0.0;
    diffL = 0.0;
    storedR[0] = storedR[1] = 0.0;
    diffR = 0.0;
    gate = 1.1;
}

void SoftGate::processStereo (const float* in1, const float* in2,
                              float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);
    const float B = param (kParamB);
    const float C = param (kParamC);

    double overallscale = 1.0;
    overallscale /= 44100.0;
    overallscale *= getSampleRate();

    double threshold = std::pow (A, 6);
    double recovery = std::pow ((B * 0.5), 6);
    recovery /= overallscale;
    double baseline = std::pow (C, 6);
    double invrec = 1.0 - recovery;

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;

        storedL[1] = storedL[0];
        storedL[0] = inputSampleL;
        diffL = storedL[0] - storedL[1];

        storedR[1] = storedR[0];
        storedR[0] = inputSampleR;
        diffR = storedR[0] - storedR[1];

        if (gate > 0) { gate = ((gate - baseline) * invrec) + baseline; }

        if ((std::fabs (diffR) > threshold) || (std::fabs (diffL) > threshold)) { gate = 1.1; }
        else
        {
            gate = (gate * invrec);
            if (threshold > 0)
            {
                gate += ((std::fabs (inputSampleL) / threshold) * recovery);
                gate += ((std::fabs (inputSampleR) / threshold) * recovery);
            }
        }

        if (gate < 0) gate = 0;

        if (gate < 1.0)
        {
            storedL[0] = storedL[1] + (diffL * gate);
            storedR[0] = storedR[1] + (diffR * gate);
        }

        if (gate < 1)
        {
            inputSampleL = (inputSampleL * gate) + (storedL[0] * (1.0 - gate));
            inputSampleR = (inputSampleR * gate) + (storedR[0] * (1.0 - gate));
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
