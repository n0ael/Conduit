/* ========================================
 *  Stonefire - Stonefire.cpp
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Stonefire —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter
 *  (entspricht dem processDoubleReplacing-Pfad des Originals).
 * ======================================== */

#include "DSP/Airwindows/Plugins/Stonefire.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "air",   "Air",   0.5f },
        { "fire",  "Fire",  0.5f },
        { "stone", "Stone", 0.5f },
        { "range", "Range", 0.5f },
    };
}

Stonefire::Stonefire() noexcept
    : AirwindowsPlugin ("stonefire", "Stonefire", kParameters)
{
}

void Stonefire::resetState() noexcept
{
    for (auto& v : air) v = 0.0;
    for (auto& v : kal) v = 0.0;

    trebleGainA = 1.0; trebleGainB = 1.0;
    midGainA = 1.0; midGainB = 1.0;
    bassGainA = 1.0; bassGainB = 1.0;
}

void Stonefire::processStereo (const float* in1, const float* in2,
                               float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);
    const float B = param (kParamB);
    const float C = param (kParamC);
    const float D = param (kParamD);

    const int inFramesToProcess = sampleFrames; //vst doesn't give us this as a separate variable so we'll make it
    double overallscale = 1.0;
    overallscale /= 44100.0;
    overallscale *= getSampleRate();
    trebleGainA = trebleGainB; trebleGainB = A * 2.0;
    midGainA = midGainB; midGainB = B * 2.0;
    bassGainA = bassGainB; bassGainB = C * 2.0;
    //simple three band to adjust
    double kalman = 1.0 - std::pow (D, 2);
    //crossover frequency between mid/bass

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;
        double drySampleL = inputSampleL;
        double drySampleR = inputSampleR;

        double temp = (double) sampleFrames / inFramesToProcess;
        double trebleGain = (trebleGainA * temp) + (trebleGainB * (1.0 - temp));
        if (trebleGain > 1.0) trebleGain = std::pow (trebleGain, 3.0 + std::sqrt (overallscale));
        if (trebleGain < 1.0) trebleGain = 1.0 - std::pow (1.0 - trebleGain, 2);

        double midGain = (midGainA * temp) + (midGainB * (1.0 - temp));
        if (midGain > 1.0) midGain *= midGain;
        if (midGain < 1.0) midGain = 1.0 - std::pow (1.0 - midGain, 2);

        double bassGain = (bassGainA * temp) + (bassGainB * (1.0 - temp));
        if (bassGain > 1.0) bassGain *= bassGain;
        if (bassGain < 1.0) bassGain = 1.0 - std::pow (1.0 - bassGain, 2);

        //begin Air3L
        air[pvSL4] = air[pvAL4] - air[pvAL3]; air[pvSL3] = air[pvAL3] - air[pvAL2];
        air[pvSL2] = air[pvAL2] - air[pvAL1]; air[pvSL1] = air[pvAL1] - inputSampleL;
        air[accSL3] = air[pvSL4] - air[pvSL3]; air[accSL2] = air[pvSL3] - air[pvSL2];
        air[accSL1] = air[pvSL2] - air[pvSL1];
        air[acc2SL2] = air[accSL3] - air[accSL2]; air[acc2SL1] = air[accSL2] - air[accSL1];
        air[outAL] = -(air[pvAL1] + air[pvSL3] + air[acc2SL2] - ((air[acc2SL2] + air[acc2SL1]) * 0.5));
        air[gainAL] *= 0.5; air[gainAL] += std::fabs (drySampleL - air[outAL]) * 0.5;
        if (air[gainAL] > 0.3 * std::sqrt (overallscale)) air[gainAL] = 0.3 * std::sqrt (overallscale);
        air[pvAL4] = air[pvAL3]; air[pvAL3] = air[pvAL2];
        air[pvAL2] = air[pvAL1]; air[pvAL1] = (air[gainAL] * air[outAL]) + drySampleL;
        double midL = drySampleL - ((air[outAL] * 0.5) + (drySampleL * (0.457 - (0.017 * overallscale))));
        temp = (midL + air[gndavgL]) * 0.5; air[gndavgL] = midL; midL = temp;
        double trebleL = drySampleL - midL;
        inputSampleL = midL;
        //end Air3L

        //begin Air3R
        air[pvSR4] = air[pvAR4] - air[pvAR3]; air[pvSR3] = air[pvAR3] - air[pvAR2];
        air[pvSR2] = air[pvAR2] - air[pvAR1]; air[pvSR1] = air[pvAR1] - inputSampleR;
        air[accSR3] = air[pvSR4] - air[pvSR3]; air[accSR2] = air[pvSR3] - air[pvSR2];
        air[accSR1] = air[pvSR2] - air[pvSR1];
        air[acc2SR2] = air[accSR3] - air[accSR2]; air[acc2SR1] = air[accSR2] - air[accSR1];
        air[outAR] = -(air[pvAR1] + air[pvSR3] + air[acc2SR2] - ((air[acc2SR2] + air[acc2SR1]) * 0.5));
        air[gainAR] *= 0.5; air[gainAR] += std::fabs (drySampleR - air[outAR]) * 0.5;
        if (air[gainAR] > 0.3 * std::sqrt (overallscale)) air[gainAR] = 0.3 * std::sqrt (overallscale);
        air[pvAR4] = air[pvAR3]; air[pvAR3] = air[pvAR2];
        air[pvAR2] = air[pvAR1]; air[pvAR1] = (air[gainAR] * air[outAR]) + drySampleR;
        double midR = drySampleR - ((air[outAR] * 0.5) + (drySampleR * (0.457 - (0.017 * overallscale))));
        temp = (midR + air[gndavgR]) * 0.5; air[gndavgR] = midR; midR = temp;
        double trebleR = drySampleR - midR;
        inputSampleR = midR;
        //end Air3R

        //begin KalmanL
        temp = inputSampleL = inputSampleL * (1.0 - kalman) * 0.777;
        inputSampleL *= (1.0 - kalman);
        //set up gain levels to control the beast
        kal[prevSlewL3] += kal[prevSampL3] - kal[prevSampL2]; kal[prevSlewL3] *= 0.5;
        kal[prevSlewL2] += kal[prevSampL2] - kal[prevSampL1]; kal[prevSlewL2] *= 0.5;
        kal[prevSlewL1] += kal[prevSampL1] - inputSampleL; kal[prevSlewL1] *= 0.5;
        //make slews from each set of samples used
        kal[accSlewL2] += kal[prevSlewL3] - kal[prevSlewL2]; kal[accSlewL2] *= 0.5;
        kal[accSlewL1] += kal[prevSlewL2] - kal[prevSlewL1]; kal[accSlewL1] *= 0.5;
        //differences between slews: rate of change of rate of change
        kal[accSlewL3] += (kal[accSlewL2] - kal[accSlewL1]); kal[accSlewL3] *= 0.5;
        //entering the abyss, what even is this
        kal[kalOutL] += kal[prevSampL1] + kal[prevSlewL2] + kal[accSlewL3]; kal[kalOutL] *= 0.5;
        //resynthesizing predicted result (all iir smoothed)
        kal[kalGainL] += std::fabs (temp - kal[kalOutL]) * kalman * 8.0; kal[kalGainL] *= 0.5;
        //madness takes its toll. Kalman Gain: how much dry to retain
        if (kal[kalGainL] > kalman * 0.5) kal[kalGainL] = kalman * 0.5;
        //attempts to avoid explosions
        kal[kalOutL] += (temp * (1.0 - (0.68 + (kalman * 0.157))));
        //this is for tuning a really complete cancellation up around Nyquist
        kal[prevSampL3] = kal[prevSampL2]; kal[prevSampL2] = kal[prevSampL1];
        kal[prevSampL1] = (kal[kalGainL] * kal[kalOutL]) + ((1.0 - kal[kalGainL]) * temp);
        //feed the chain of previous samples
        if (kal[prevSampL1] > 1.0) kal[prevSampL1] = 1.0; if (kal[prevSampL1] < -1.0) kal[prevSampL1] = -1.0;
        double bassL = kal[kalOutL] * 0.777;
        midL -= bassL;
        //end KalmanL

        //begin KalmanR
        temp = inputSampleR = inputSampleR * (1.0 - kalman) * 0.777;
        inputSampleR *= (1.0 - kalman);
        //set up gain levels to control the beast
        kal[prevSlewR3] += kal[prevSampR3] - kal[prevSampR2]; kal[prevSlewR3] *= 0.5;
        kal[prevSlewR2] += kal[prevSampR2] - kal[prevSampR1]; kal[prevSlewR2] *= 0.5;
        kal[prevSlewR1] += kal[prevSampR1] - inputSampleR; kal[prevSlewR1] *= 0.5;
        //make slews from each set of samples used
        kal[accSlewR2] += kal[prevSlewR3] - kal[prevSlewR2]; kal[accSlewR2] *= 0.5;
        kal[accSlewR1] += kal[prevSlewR2] - kal[prevSlewR1]; kal[accSlewR1] *= 0.5;
        //differences between slews: rate of change of rate of change
        kal[accSlewR3] += (kal[accSlewR2] - kal[accSlewR1]); kal[accSlewR3] *= 0.5;
        //entering the abyss, what even is this
        kal[kalOutR] += kal[prevSampR1] + kal[prevSlewR2] + kal[accSlewR3]; kal[kalOutR] *= 0.5;
        //resynthesizing predicted result (all iir smoothed)
        kal[kalGainR] += std::fabs (temp - kal[kalOutR]) * kalman * 8.0; kal[kalGainR] *= 0.5;
        //madness takes its toll. Kalman Gain: how much dry to retain
        if (kal[kalGainR] > kalman * 0.5) kal[kalGainR] = kalman * 0.5;
        //attempts to avoid explosions
        kal[kalOutR] += (temp * (1.0 - (0.68 + (kalman * 0.157))));
        //this is for tuning a really complete cancellation up around Nyquist
        kal[prevSampR3] = kal[prevSampR2]; kal[prevSampR2] = kal[prevSampR1];
        kal[prevSampR1] = (kal[kalGainR] * kal[kalOutR]) + ((1.0 - kal[kalGainR]) * temp);
        //feed the chain of previous samples
        if (kal[prevSampR1] > 1.0) kal[prevSampR1] = 1.0; if (kal[prevSampR1] < -1.0) kal[prevSampR1] = -1.0;
        double bassR = kal[kalOutR] * 0.777;
        midR -= bassR;
        //end KalmanR

        inputSampleL = (bassL * bassGain) + (midL * midGain) + (trebleL * trebleGain);
        inputSampleR = (bassR * bassGain) + (midR * midGain) + (trebleR * trebleGain);
        //applies pan section, and smoothed fader gain

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
