/* ========================================
 *  DubSub2 - DubSub2.cpp
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/DubSub2 —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter.
 * ======================================== */

#include "DSP/Airwindows/Plugins/DubSub2.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "head_bump", "HeadBmp", 0.5f },
        { "head_freq", "HeadFrq", 0.5f },
        { "dry_wet",   "Dry/Wet", 0.5f },
    };
}

DubSub2::DubSub2() noexcept
    : AirwindowsPlugin ("dubsub2", "DubSub2", kParameters)
{
}

void DubSub2::resetState() noexcept
{
    headBumpL = 0.0;
    headBumpR = 0.0;
    for (double& v : hdbA) v = 0.0;
    for (double& v : hdbB) v = 0.0;
}

void DubSub2::processStereo (const float* in1, const float* in2,
                             float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);
    const float B = param (kParamB);
    const float C = param (kParamC);

    double overallscale = 1.0;
    overallscale /= 44100.0;
    overallscale *= getSampleRate();

    double headBumpDrive = (A * 0.1) / overallscale;

    hdbA[hdb_freq] = (((B * B) * 175.0) + 25.0) / getSampleRate();
    hdbB[hdb_freq] = hdbA[hdb_freq] * 0.9375;
    hdbB[hdb_reso] = hdbA[hdb_reso] = 0.618033988749894848204586;
    hdbB[hdb_a1] = hdbA[hdb_a1] = 0.0;

    double K = std::tan (juce::MathConstants<double>::pi * hdbA[hdb_freq]);
    double norm = 1.0 / (1.0 + K / hdbA[hdb_reso] + K * K);
    hdbA[hdb_a0] = K / hdbA[hdb_reso] * norm;
    hdbA[hdb_a2] = -hdbA[hdb_a0];
    hdbA[hdb_b1] = 2.0 * (K * K - 1.0) * norm;
    hdbA[hdb_b2] = (1.0 - K / hdbA[hdb_reso] + K * K) * norm;
    K = std::tan (juce::MathConstants<double>::pi * hdbB[hdb_freq]);
    norm = 1.0 / (1.0 + K / hdbB[hdb_reso] + K * K);
    hdbB[hdb_a0] = K / hdbB[hdb_reso] * norm;
    hdbB[hdb_a2] = -hdbB[hdb_a0];
    hdbB[hdb_b1] = 2.0 * (K * K - 1.0) * norm;
    hdbB[hdb_b2] = (1.0 - K / hdbB[hdb_reso] + K * K) * norm;

    double headWet = C;

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;
        double drySampleL = inputSampleL;
        double drySampleR = inputSampleR;

        //begin HeadBump
        headBumpL += (inputSampleL * headBumpDrive);
        headBumpL -= (headBumpL * headBumpL * headBumpL * (0.0618 / std::sqrt (overallscale)));
        headBumpR += (inputSampleR * headBumpDrive);
        headBumpR -= (headBumpR * headBumpR * headBumpR * (0.0618 / std::sqrt (overallscale)));
        double headBiqSampleL = (headBumpL * hdbA[hdb_a0]) + hdbA[hdb_sL1];
        hdbA[hdb_sL1] = (headBumpL * hdbA[hdb_a1]) - (headBiqSampleL * hdbA[hdb_b1]) + hdbA[hdb_sL2];
        hdbA[hdb_sL2] = (headBumpL * hdbA[hdb_a2]) - (headBiqSampleL * hdbA[hdb_b2]);
        double headBumpSampleL = (headBiqSampleL * hdbB[hdb_a0]) + hdbB[hdb_sL1];
        hdbB[hdb_sL1] = (headBiqSampleL * hdbB[hdb_a1]) - (headBumpSampleL * hdbB[hdb_b1]) + hdbB[hdb_sL2];
        hdbB[hdb_sL2] = (headBiqSampleL * hdbB[hdb_a2]) - (headBumpSampleL * hdbB[hdb_b2]);
        double headBiqSampleR = (headBumpR * hdbA[hdb_a0]) + hdbA[hdb_sR1];
        hdbA[hdb_sR1] = (headBumpR * hdbA[hdb_a1]) - (headBiqSampleR * hdbA[hdb_b1]) + hdbA[hdb_sR2];
        hdbA[hdb_sR2] = (headBumpR * hdbA[hdb_a2]) - (headBiqSampleR * hdbA[hdb_b2]);
        double headBumpSampleR = (headBiqSampleR * hdbB[hdb_a0]) + hdbB[hdb_sR1];
        hdbB[hdb_sR1] = (headBiqSampleR * hdbB[hdb_a1]) - (headBumpSampleR * hdbB[hdb_b1]) + hdbB[hdb_sR2];
        hdbB[hdb_sR2] = (headBiqSampleR * hdbB[hdb_a2]) - (headBumpSampleR * hdbB[hdb_b2]);
        //end HeadBump

        inputSampleL = (headBumpSampleL * headWet) + (drySampleL * (1.0 - headWet));
        inputSampleR = (headBumpSampleR * headWet) + (drySampleR * (1.0 - headWet));

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
