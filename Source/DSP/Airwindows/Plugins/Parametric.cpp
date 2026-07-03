/* ========================================
 *  Parametric - Parametric.cpp
 *  Copyright (c) Airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Parametric —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter
 *  (entspricht dem processDoubleReplacing-Pfad des Originals).
 * ======================================== */

#include "DSP/Airwindows/Plugins/Parametric.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "tr_freq", "Tr Freq", 0.5f },
        { "treble",  "Treble",  0.5f },
        { "tr_reso", "Tr Reso", 0.5f },
        { "hm_freq", "HM Freq", 0.5f },
        { "highmid", "HighMid", 0.5f },
        { "hm_reso", "HM Reso", 0.5f },
        { "lm_freq", "LM Freq", 0.5f },
        { "lowmid",  "LowMid",  0.5f },
        { "lm_reso", "LM Reso", 0.5f },
        { "dry_wet", "Dry/Wet", 1.0f },
    };
}

Parametric::Parametric() noexcept
    : AirwindowsPlugin ("parametric", "Parametric", kParameters)
{
}

void Parametric::resetState() noexcept
{
    for (auto& v : high) v = 0.0;
    for (auto& v : hmid) v = 0.0;
    for (auto& v : lmid) v = 0.0;
}

void Parametric::processStereo (const float* in1, const float* in2,
                                float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);
    const float B = param (kParamB);
    const float C = param (kParamC);
    const float D = param (kParamD);
    const float E = param (kParamE);
    const float F = param (kParamF);
    const float G = param (kParamG);
    const float H = param (kParamH);
    const float I = param (kParamI);
    const float J = param (kParamJ);

    high[biqs_freq] = (((std::pow (A, 3) * 14500.0) + 1500.0) / getSampleRate());
    if (high[biqs_freq] < 0.0001) high[biqs_freq] = 0.0001;
    high[biqs_nonlin] = B;
    high[biqs_level] = (high[biqs_nonlin] * 2.0) - 1.0;
    if (high[biqs_level] > 0.0) high[biqs_level] *= 2.0;
    high[biqs_reso] = ((0.5 + (high[biqs_nonlin] * 0.5) + std::sqrt (high[biqs_freq])) - (1.0 - std::pow (1.0 - C, 2.0))) + 0.5 + (high[biqs_nonlin] * 0.5);
    double K = std::tan (juce::MathConstants<double>::pi * high[biqs_freq]);
    double norm = 1.0 / (1.0 + K / (high[biqs_reso] * 1.93185165) + K * K);
    high[biqs_a0] = K / (high[biqs_reso] * 1.93185165) * norm;
    high[biqs_b1] = 2.0 * (K * K - 1.0) * norm;
    high[biqs_b2] = (1.0 - K / (high[biqs_reso] * 1.93185165) + K * K) * norm;
    norm = 1.0 / (1.0 + K / (high[biqs_reso] * 0.70710678) + K * K);
    high[biqs_c0] = K / (high[biqs_reso] * 0.70710678) * norm;
    high[biqs_d1] = 2.0 * (K * K - 1.0) * norm;
    high[biqs_d2] = (1.0 - K / (high[biqs_reso] * 0.70710678) + K * K) * norm;
    norm = 1.0 / (1.0 + K / (high[biqs_reso] * 0.51763809) + K * K);
    high[biqs_e0] = K / (high[biqs_reso] * 0.51763809) * norm;
    high[biqs_f1] = 2.0 * (K * K - 1.0) * norm;
    high[biqs_f2] = (1.0 - K / (high[biqs_reso] * 0.51763809) + K * K) * norm;
    //high

    hmid[biqs_freq] = (((std::pow (D, 3) * 6400.0) + 600.0) / getSampleRate());
    if (hmid[biqs_freq] < 0.0001) hmid[biqs_freq] = 0.0001;
    hmid[biqs_nonlin] = E;
    hmid[biqs_level] = (hmid[biqs_nonlin] * 2.0) - 1.0;
    if (hmid[biqs_level] > 0.0) hmid[biqs_level] *= 2.0;
    hmid[biqs_reso] = ((0.5 + (hmid[biqs_nonlin] * 0.5) + std::sqrt (hmid[biqs_freq])) - (1.0 - std::pow (1.0 - F, 2.0))) + 0.5 + (hmid[biqs_nonlin] * 0.5);
    K = std::tan (juce::MathConstants<double>::pi * hmid[biqs_freq]);
    norm = 1.0 / (1.0 + K / (hmid[biqs_reso] * 1.93185165) + K * K);
    hmid[biqs_a0] = K / (hmid[biqs_reso] * 1.93185165) * norm;
    hmid[biqs_b1] = 2.0 * (K * K - 1.0) * norm;
    hmid[biqs_b2] = (1.0 - K / (hmid[biqs_reso] * 1.93185165) + K * K) * norm;
    norm = 1.0 / (1.0 + K / (hmid[biqs_reso] * 0.70710678) + K * K);
    hmid[biqs_c0] = K / (hmid[biqs_reso] * 0.70710678) * norm;
    hmid[biqs_d1] = 2.0 * (K * K - 1.0) * norm;
    hmid[biqs_d2] = (1.0 - K / (hmid[biqs_reso] * 0.70710678) + K * K) * norm;
    norm = 1.0 / (1.0 + K / (hmid[biqs_reso] * 0.51763809) + K * K);
    hmid[biqs_e0] = K / (hmid[biqs_reso] * 0.51763809) * norm;
    hmid[biqs_f1] = 2.0 * (K * K - 1.0) * norm;
    hmid[biqs_f2] = (1.0 - K / (hmid[biqs_reso] * 0.51763809) + K * K) * norm;
    //hmid

    lmid[biqs_freq] = (((std::pow (G, 3) * 2200.0) + 20.0) / getSampleRate());
    if (lmid[biqs_freq] < 0.00001) lmid[biqs_freq] = 0.00001;
    lmid[biqs_nonlin] = H;
    lmid[biqs_level] = (lmid[biqs_nonlin] * 2.0) - 1.0;
    if (lmid[biqs_level] > 0.0) lmid[biqs_level] *= 2.0;
    lmid[biqs_reso] = ((0.5 + (lmid[biqs_nonlin] * 0.5) + std::sqrt (lmid[biqs_freq])) - (1.0 - std::pow (1.0 - I, 2.0))) + 0.5 + (lmid[biqs_nonlin] * 0.5);
    K = std::tan (juce::MathConstants<double>::pi * lmid[biqs_freq]);
    norm = 1.0 / (1.0 + K / (lmid[biqs_reso] * 1.93185165) + K * K);
    lmid[biqs_a0] = K / (lmid[biqs_reso] * 1.93185165) * norm;
    lmid[biqs_b1] = 2.0 * (K * K - 1.0) * norm;
    lmid[biqs_b2] = (1.0 - K / (lmid[biqs_reso] * 1.93185165) + K * K) * norm;
    norm = 1.0 / (1.0 + K / (lmid[biqs_reso] * 0.70710678) + K * K);
    lmid[biqs_c0] = K / (lmid[biqs_reso] * 0.70710678) * norm;
    lmid[biqs_d1] = 2.0 * (K * K - 1.0) * norm;
    lmid[biqs_d2] = (1.0 - K / (lmid[biqs_reso] * 0.70710678) + K * K) * norm;
    norm = 1.0 / (1.0 + K / (lmid[biqs_reso] * 0.51763809) + K * K);
    lmid[biqs_e0] = K / (lmid[biqs_reso] * 0.51763809) * norm;
    lmid[biqs_f1] = 2.0 * (K * K - 1.0) * norm;
    lmid[biqs_f2] = (1.0 - K / (lmid[biqs_reso] * 0.51763809) + K * K) * norm;
    //lmid

    double wet = J;

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;

        //begin Stacked Biquad With Reversed Neutron Flow L
        high[biqs_outL] = inputSampleL * std::fabs (high[biqs_level]);
        high[biqs_dis] = std::fabs (high[biqs_a0] * (1.0 + (high[biqs_outL] * high[biqs_nonlin])));
        if (high[biqs_dis] > 1.0) high[biqs_dis] = 1.0;
        high[biqs_temp] = (high[biqs_outL] * high[biqs_dis]) + high[biqs_aL1];
        high[biqs_aL1] = high[biqs_aL2] - (high[biqs_temp] * high[biqs_b1]);
        high[biqs_aL2] = (high[biqs_outL] * -high[biqs_dis]) - (high[biqs_temp] * high[biqs_b2]);
        high[biqs_outL] = high[biqs_temp];
        high[biqs_dis] = std::fabs (high[biqs_c0] * (1.0 + (high[biqs_outL] * high[biqs_nonlin])));
        if (high[biqs_dis] > 1.0) high[biqs_dis] = 1.0;
        high[biqs_temp] = (high[biqs_outL] * high[biqs_dis]) + high[biqs_cL1];
        high[biqs_cL1] = high[biqs_cL2] - (high[biqs_temp] * high[biqs_d1]);
        high[biqs_cL2] = (high[biqs_outL] * -high[biqs_dis]) - (high[biqs_temp] * high[biqs_d2]);
        high[biqs_outL] = high[biqs_temp];
        high[biqs_dis] = std::fabs (high[biqs_e0] * (1.0 + (high[biqs_outL] * high[biqs_nonlin])));
        if (high[biqs_dis] > 1.0) high[biqs_dis] = 1.0;
        high[biqs_temp] = (high[biqs_outL] * high[biqs_dis]) + high[biqs_eL1];
        high[biqs_eL1] = high[biqs_eL2] - (high[biqs_temp] * high[biqs_f1]);
        high[biqs_eL2] = (high[biqs_outL] * -high[biqs_dis]) - (high[biqs_temp] * high[biqs_f2]);
        high[biqs_outL] = high[biqs_temp]; high[biqs_outL] *= high[biqs_level];
        if (high[biqs_level] > 1.0) high[biqs_outL] *= high[biqs_level];
        //end Stacked Biquad With Reversed Neutron Flow L

        //begin Stacked Biquad With Reversed Neutron Flow L
        hmid[biqs_outL] = inputSampleL * std::fabs (hmid[biqs_level]);
        hmid[biqs_dis] = std::fabs (hmid[biqs_a0] * (1.0 + (hmid[biqs_outL] * hmid[biqs_nonlin])));
        if (hmid[biqs_dis] > 1.0) hmid[biqs_dis] = 1.0;
        hmid[biqs_temp] = (hmid[biqs_outL] * hmid[biqs_dis]) + hmid[biqs_aL1];
        hmid[biqs_aL1] = hmid[biqs_aL2] - (hmid[biqs_temp] * hmid[biqs_b1]);
        hmid[biqs_aL2] = (hmid[biqs_outL] * -hmid[biqs_dis]) - (hmid[biqs_temp] * hmid[biqs_b2]);
        hmid[biqs_outL] = hmid[biqs_temp];
        hmid[biqs_dis] = std::fabs (hmid[biqs_c0] * (1.0 + (hmid[biqs_outL] * hmid[biqs_nonlin])));
        if (hmid[biqs_dis] > 1.0) hmid[biqs_dis] = 1.0;
        hmid[biqs_temp] = (hmid[biqs_outL] * hmid[biqs_dis]) + hmid[biqs_cL1];
        hmid[biqs_cL1] = hmid[biqs_cL2] - (hmid[biqs_temp] * hmid[biqs_d1]);
        hmid[biqs_cL2] = (hmid[biqs_outL] * -hmid[biqs_dis]) - (hmid[biqs_temp] * hmid[biqs_d2]);
        hmid[biqs_outL] = hmid[biqs_temp];
        hmid[biqs_dis] = std::fabs (hmid[biqs_e0] * (1.0 + (hmid[biqs_outL] * hmid[biqs_nonlin])));
        if (hmid[biqs_dis] > 1.0) hmid[biqs_dis] = 1.0;
        hmid[biqs_temp] = (hmid[biqs_outL] * hmid[biqs_dis]) + hmid[biqs_eL1];
        hmid[biqs_eL1] = hmid[biqs_eL2] - (hmid[biqs_temp] * hmid[biqs_f1]);
        hmid[biqs_eL2] = (hmid[biqs_outL] * -hmid[biqs_dis]) - (hmid[biqs_temp] * hmid[biqs_f2]);
        hmid[biqs_outL] = hmid[biqs_temp]; hmid[biqs_outL] *= hmid[biqs_level];
        if (hmid[biqs_level] > 1.0) hmid[biqs_outL] *= hmid[biqs_level];
        //end Stacked Biquad With Reversed Neutron Flow L

        //begin Stacked Biquad With Reversed Neutron Flow L
        lmid[biqs_outL] = inputSampleL * std::fabs (lmid[biqs_level]);
        lmid[biqs_dis] = std::fabs (lmid[biqs_a0] * (1.0 + (lmid[biqs_outL] * lmid[biqs_nonlin])));
        if (lmid[biqs_dis] > 1.0) lmid[biqs_dis] = 1.0;
        lmid[biqs_temp] = (lmid[biqs_outL] * lmid[biqs_dis]) + lmid[biqs_aL1];
        lmid[biqs_aL1] = lmid[biqs_aL2] - (lmid[biqs_temp] * lmid[biqs_b1]);
        lmid[biqs_aL2] = (lmid[biqs_outL] * -lmid[biqs_dis]) - (lmid[biqs_temp] * lmid[biqs_b2]);
        lmid[biqs_outL] = lmid[biqs_temp];
        lmid[biqs_dis] = std::fabs (lmid[biqs_c0] * (1.0 + (lmid[biqs_outL] * lmid[biqs_nonlin])));
        if (lmid[biqs_dis] > 1.0) lmid[biqs_dis] = 1.0;
        lmid[biqs_temp] = (lmid[biqs_outL] * lmid[biqs_dis]) + lmid[biqs_cL1];
        lmid[biqs_cL1] = lmid[biqs_cL2] - (lmid[biqs_temp] * lmid[biqs_d1]);
        lmid[biqs_cL2] = (lmid[biqs_outL] * -lmid[biqs_dis]) - (lmid[biqs_temp] * lmid[biqs_d2]);
        lmid[biqs_outL] = lmid[biqs_temp];
        lmid[biqs_dis] = std::fabs (lmid[biqs_e0] * (1.0 + (lmid[biqs_outL] * lmid[biqs_nonlin])));
        if (lmid[biqs_dis] > 1.0) lmid[biqs_dis] = 1.0;
        lmid[biqs_temp] = (lmid[biqs_outL] * lmid[biqs_dis]) + lmid[biqs_eL1];
        lmid[biqs_eL1] = lmid[biqs_eL2] - (lmid[biqs_temp] * lmid[biqs_f1]);
        lmid[biqs_eL2] = (lmid[biqs_outL] * -lmid[biqs_dis]) - (lmid[biqs_temp] * lmid[biqs_f2]);
        lmid[biqs_outL] = lmid[biqs_temp]; lmid[biqs_outL] *= lmid[biqs_level];
        if (lmid[biqs_level] > 1.0) lmid[biqs_outL] *= lmid[biqs_level];
        //end Stacked Biquad With Reversed Neutron Flow L

        //begin Stacked Biquad With Reversed Neutron Flow R
        high[biqs_outR] = inputSampleR * std::fabs (high[biqs_level]);
        high[biqs_dis] = std::fabs (high[biqs_a0] * (1.0 + (high[biqs_outR] * high[biqs_nonlin])));
        if (high[biqs_dis] > 1.0) high[biqs_dis] = 1.0;
        high[biqs_temp] = (high[biqs_outR] * high[biqs_dis]) + high[biqs_aR1];
        high[biqs_aR1] = high[biqs_aR2] - (high[biqs_temp] * high[biqs_b1]);
        high[biqs_aR2] = (high[biqs_outR] * -high[biqs_dis]) - (high[biqs_temp] * high[biqs_b2]);
        high[biqs_outR] = high[biqs_temp];
        high[biqs_dis] = std::fabs (high[biqs_c0] * (1.0 + (high[biqs_outR] * high[biqs_nonlin])));
        if (high[biqs_dis] > 1.0) high[biqs_dis] = 1.0;
        high[biqs_temp] = (high[biqs_outR] * high[biqs_dis]) + high[biqs_cR1];
        high[biqs_cR1] = high[biqs_cR2] - (high[biqs_temp] * high[biqs_d1]);
        high[biqs_cR2] = (high[biqs_outR] * -high[biqs_dis]) - (high[biqs_temp] * high[biqs_d2]);
        high[biqs_outR] = high[biqs_temp];
        high[biqs_dis] = std::fabs (high[biqs_e0] * (1.0 + (high[biqs_outR] * high[biqs_nonlin])));
        if (high[biqs_dis] > 1.0) high[biqs_dis] = 1.0;
        high[biqs_temp] = (high[biqs_outR] * high[biqs_dis]) + high[biqs_eR1];
        high[biqs_eR1] = high[biqs_eR2] - (high[biqs_temp] * high[biqs_f1]);
        high[biqs_eR2] = (high[biqs_outR] * -high[biqs_dis]) - (high[biqs_temp] * high[biqs_f2]);
        high[biqs_outR] = high[biqs_temp]; high[biqs_outR] *= high[biqs_level];
        if (high[biqs_level] > 1.0) high[biqs_outR] *= high[biqs_level];
        //end Stacked Biquad With Reversed Neutron Flow R

        //begin Stacked Biquad With Reversed Neutron Flow R
        hmid[biqs_outR] = inputSampleR * std::fabs (hmid[biqs_level]);
        hmid[biqs_dis] = std::fabs (hmid[biqs_a0] * (1.0 + (hmid[biqs_outR] * hmid[biqs_nonlin])));
        if (hmid[biqs_dis] > 1.0) hmid[biqs_dis] = 1.0;
        hmid[biqs_temp] = (hmid[biqs_outR] * hmid[biqs_dis]) + hmid[biqs_aR1];
        hmid[biqs_aR1] = hmid[biqs_aR2] - (hmid[biqs_temp] * hmid[biqs_b1]);
        hmid[biqs_aR2] = (hmid[biqs_outR] * -hmid[biqs_dis]) - (hmid[biqs_temp] * hmid[biqs_b2]);
        hmid[biqs_outR] = hmid[biqs_temp];
        hmid[biqs_dis] = std::fabs (hmid[biqs_c0] * (1.0 + (hmid[biqs_outR] * hmid[biqs_nonlin])));
        if (hmid[biqs_dis] > 1.0) hmid[biqs_dis] = 1.0;
        hmid[biqs_temp] = (hmid[biqs_outR] * hmid[biqs_dis]) + hmid[biqs_cR1];
        hmid[biqs_cR1] = hmid[biqs_cR2] - (hmid[biqs_temp] * hmid[biqs_d1]);
        hmid[biqs_cR2] = (hmid[biqs_outR] * -hmid[biqs_dis]) - (hmid[biqs_temp] * hmid[biqs_d2]);
        hmid[biqs_outR] = hmid[biqs_temp];
        hmid[biqs_dis] = std::fabs (hmid[biqs_e0] * (1.0 + (hmid[biqs_outR] * hmid[biqs_nonlin])));
        if (hmid[biqs_dis] > 1.0) hmid[biqs_dis] = 1.0;
        hmid[biqs_temp] = (hmid[biqs_outR] * hmid[biqs_dis]) + hmid[biqs_eR1];
        hmid[biqs_eR1] = hmid[biqs_eR2] - (hmid[biqs_temp] * hmid[biqs_f1]);
        hmid[biqs_eR2] = (hmid[biqs_outR] * -hmid[biqs_dis]) - (hmid[biqs_temp] * hmid[biqs_f2]);
        hmid[biqs_outR] = hmid[biqs_temp]; hmid[biqs_outR] *= hmid[biqs_level];
        if (hmid[biqs_level] > 1.0) hmid[biqs_outR] *= hmid[biqs_level];
        //end Stacked Biquad With Reversed Neutron Flow R

        //begin Stacked Biquad With Reversed Neutron Flow R
        lmid[biqs_outR] = inputSampleR * std::fabs (lmid[biqs_level]);
        lmid[biqs_dis] = std::fabs (lmid[biqs_a0] * (1.0 + (lmid[biqs_outR] * lmid[biqs_nonlin])));
        if (lmid[biqs_dis] > 1.0) lmid[biqs_dis] = 1.0;
        lmid[biqs_temp] = (lmid[biqs_outR] * lmid[biqs_dis]) + lmid[biqs_aR1];
        lmid[biqs_aR1] = lmid[biqs_aR2] - (lmid[biqs_temp] * lmid[biqs_b1]);
        lmid[biqs_aR2] = (lmid[biqs_outR] * -lmid[biqs_dis]) - (lmid[biqs_temp] * lmid[biqs_b2]);
        lmid[biqs_outR] = lmid[biqs_temp];
        lmid[biqs_dis] = std::fabs (lmid[biqs_c0] * (1.0 + (lmid[biqs_outR] * lmid[biqs_nonlin])));
        if (lmid[biqs_dis] > 1.0) lmid[biqs_dis] = 1.0;
        lmid[biqs_temp] = (lmid[biqs_outR] * lmid[biqs_dis]) + lmid[biqs_cR1];
        lmid[biqs_cR1] = lmid[biqs_cR2] - (lmid[biqs_temp] * lmid[biqs_d1]);
        lmid[biqs_cR2] = (lmid[biqs_outR] * -lmid[biqs_dis]) - (lmid[biqs_temp] * lmid[biqs_d2]);
        lmid[biqs_outR] = lmid[biqs_temp];
        lmid[biqs_dis] = std::fabs (lmid[biqs_e0] * (1.0 + (lmid[biqs_outR] * lmid[biqs_nonlin])));
        if (lmid[biqs_dis] > 1.0) lmid[biqs_dis] = 1.0;
        lmid[biqs_temp] = (lmid[biqs_outR] * lmid[biqs_dis]) + lmid[biqs_eR1];
        lmid[biqs_eR1] = lmid[biqs_eR2] - (lmid[biqs_temp] * lmid[biqs_f1]);
        lmid[biqs_eR2] = (lmid[biqs_outR] * -lmid[biqs_dis]) - (lmid[biqs_temp] * lmid[biqs_f2]);
        lmid[biqs_outR] = lmid[biqs_temp]; lmid[biqs_outR] *= lmid[biqs_level];
        if (lmid[biqs_level] > 1.0) lmid[biqs_outR] *= lmid[biqs_level];
        //end Stacked Biquad With Reversed Neutron Flow R

        double parametric = high[biqs_outL] + hmid[biqs_outL] + lmid[biqs_outL];
        inputSampleL += (parametric * wet); //purely a parallel filter stage here
        parametric = high[biqs_outR] + hmid[biqs_outR] + lmid[biqs_outR];
        inputSampleR += (parametric * wet); //purely a parallel filter stage here

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
