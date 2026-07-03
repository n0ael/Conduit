/* ========================================
 *  SmoothEQ3 - SmoothEQ3.cpp
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/SmoothEQ3 —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Kein fpd-Dither-Add im Original-Ausgang (nur der Xorshift-Advance) —
 *  hier hinter ditherOn() ergänzt (Projekt-Konvention), Xorshift läuft immer.
 * ======================================== */

#include "DSP/Airwindows/Plugins/SmoothEQ3.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "high", "High", 0.5f },
        { "mid",  "Mid",  0.5f },
        { "bass", "Bass", 0.5f },
    };
}

SmoothEQ3::SmoothEQ3() noexcept
    : AirwindowsPlugin ("smootheq3", "SmoothEQ3", kParameters)
{
}

void SmoothEQ3::resetState() noexcept
{
    for (auto& v : highFast) v = 0.0;
    for (auto& v : lowFast) v = 0.0;
    highFastLIIR = 0.0;
    highFastRIIR = 0.0;
    lowFastLIIR = 0.0;
    lowFastRIIR = 0.0;
}

void SmoothEQ3::processStereo (const float* in1, const float* in2,
                               float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);
    const float B = param (kParamB);
    const float C = param (kParamC);

    double trebleGain = (A - 0.5) * 2.0;
    trebleGain = 1.0 + (trebleGain * std::fabs (trebleGain) * std::fabs (trebleGain));
    double midGain = (B - 0.5) * 2.0;
    midGain = 1.0 + (midGain * std::fabs (midGain) * std::fabs (midGain));
    double bassGain = (C - 0.5) * 2.0;
    bassGain = 1.0 + (bassGain * std::fabs (bassGain) * std::fabs (bassGain));
    //separate from filtering stage, this is amplitude, centered on 1.0 unity gain

    //SmoothEQ3 is how to get 3rd order steepness at very low CPU.
    //because sample rate varies, you could also vary the crossovers
    //you can't vary Q because math is simplified to take advantage of
    //how the accurate Q value for this filter is always exactly 1.0.
    highFast[biq_freq] = (4000.0 / getSampleRate());
    double omega = 2.0 * juce::MathConstants<double>::pi * (4000.0 / getSampleRate()); //mid-high crossover freq
    double K = 2.0 - std::cos (omega);
    double highCoef = -std::sqrt (K * K - 1.0) + K;
    lowFast[biq_freq] = (200.0 / getSampleRate());
    omega = 2.0 * juce::MathConstants<double>::pi * (200.0 / getSampleRate()); //low-mid crossover freq
    K = 2.0 - std::cos (omega);
    double lowCoef = -std::sqrt (K * K - 1.0) + K;
    //exponential IIR filter as part of an accurate 3rd order Butterworth filter
    K = std::tan (juce::MathConstants<double>::pi * highFast[biq_freq]);
    double norm = 1.0 / (1.0 + K + K * K);
    highFast[biq_a0] = K * K * norm;
    highFast[biq_a1] = 2.0 * highFast[biq_a0];
    highFast[biq_a2] = highFast[biq_a0];
    highFast[biq_b1] = 2.0 * (K * K - 1.0) * norm;
    highFast[biq_b2] = (1.0 - K + K * K) * norm;
    K = std::tan (juce::MathConstants<double>::pi * lowFast[biq_freq]);
    norm = 1.0 / (1.0 + K + K * K);
    lowFast[biq_a0] = K * K * norm;
    lowFast[biq_a1] = 2.0 * lowFast[biq_a0];
    lowFast[biq_a2] = lowFast[biq_a0];
    lowFast[biq_b1] = 2.0 * (K * K - 1.0) * norm;
    lowFast[biq_b2] = (1.0 - K + K * K) * norm;
    //custom biquad setup with Q = 1.0 gets to omit some divides

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;

        double trebleFastL = inputSampleL;
        double outSample = (trebleFastL * highFast[biq_a0]) + highFast[biq_sL1];
        highFast[biq_sL1] = (trebleFastL * highFast[biq_a1]) - (outSample * highFast[biq_b1]) + highFast[biq_sL2];
        highFast[biq_sL2] = (trebleFastL * highFast[biq_a2]) - (outSample * highFast[biq_b2]);
        double midFastL = outSample; trebleFastL -= midFastL;
        outSample = (midFastL * lowFast[biq_a0]) + lowFast[biq_sL1];
        lowFast[biq_sL1] = (midFastL * lowFast[biq_a1]) - (outSample * lowFast[biq_b1]) + lowFast[biq_sL2];
        lowFast[biq_sL2] = (midFastL * lowFast[biq_a2]) - (outSample * lowFast[biq_b2]);
        double bassFastL = outSample; midFastL -= bassFastL;
        trebleFastL = (bassFastL * bassGain) + (midFastL * midGain) + (trebleFastL * trebleGain);
        //first stage of two crossovers is biquad of exactly 1.0 Q
        highFastLIIR = (highFastLIIR * highCoef) + (trebleFastL * (1.0 - highCoef));
        midFastL = highFastLIIR; trebleFastL -= midFastL;
        lowFastLIIR = (lowFastLIIR * lowCoef) + (midFastL * (1.0 - lowCoef));
        bassFastL = lowFastLIIR; midFastL -= bassFastL;
        inputSampleL = (bassFastL * bassGain) + (midFastL * midGain) + (trebleFastL * trebleGain);
        //second stage of two crossovers is the exponential filters
        //this produces a slightly steeper Butterworth filter very cheaply

        double trebleFastR = inputSampleR;
        outSample = (trebleFastR * highFast[biq_a0]) + highFast[biq_sR1];
        highFast[biq_sR1] = (trebleFastR * highFast[biq_a1]) - (outSample * highFast[biq_b1]) + highFast[biq_sR2];
        highFast[biq_sR2] = (trebleFastR * highFast[biq_a2]) - (outSample * highFast[biq_b2]);
        double midFastR = outSample; trebleFastR -= midFastR;
        outSample = (midFastR * lowFast[biq_a0]) + lowFast[biq_sR1];
        lowFast[biq_sR1] = (midFastR * lowFast[biq_a1]) - (outSample * lowFast[biq_b1]) + lowFast[biq_sR2];
        lowFast[biq_sR2] = (midFastR * lowFast[biq_a2]) - (outSample * lowFast[biq_b2]);
        double bassFastR = outSample; midFastR -= bassFastR;
        trebleFastR = (bassFastR * bassGain) + (midFastR * midGain) + (trebleFastR * trebleGain);
        //first stage of two crossovers is biquad of exactly 1.0 Q
        highFastRIIR = (highFastRIIR * highCoef) + (trebleFastR * (1.0 - highCoef));
        midFastR = highFastRIIR; trebleFastR -= midFastR;
        lowFastRIIR = (lowFastRIIR * lowCoef) + (midFastR * (1.0 - lowCoef));
        bassFastR = lowFastRIIR; midFastR -= bassFastR;
        inputSampleR = (bassFastR * bassGain) + (midFastR * midGain) + (trebleFastR * trebleGain);
        //second stage of two crossovers is the exponential filters
        //this produces a slightly steeper Butterworth filter very cheaply

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
