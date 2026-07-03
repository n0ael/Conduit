/* ========================================
 *  StoneFireComp - StoneFireComp.cpp
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/StoneFireComp —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter
 *  (entspricht dem processDoubleReplacing-Pfad des Originals).
 * ======================================== */

#include "DSP/Airwindows/Plugins/StoneFireComp.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "fire_th",  "Fire Th", 1.0f },
        { "attack_f", "Attack",  0.5f },
        { "release_f","Release", 0.5f },
        { "fire",     "Fire",    0.5f },
        { "stone_th", "StoneTh", 1.0f },
        { "attack_s", "Attack",  0.5f },
        { "release_s","Release", 0.5f },
        { "stone",    "Stone",   0.5f },
        { "range",    "Range",   0.5f },
        { "ratio",    "Ratio",   1.0f },
    };
}

StoneFireComp::StoneFireComp() noexcept
    : AirwindowsPlugin ("stonefirecomp", "StoneFireComp", kParameters)
{
}

void StoneFireComp::resetState() noexcept
{
    for (auto& v : kal) v = 0.0;
    fireCompL = 1.0;
    fireCompR = 1.0;
    stoneCompL = 1.0;
    stoneCompR = 1.0;
}

void StoneFireComp::processStereo (const float* in1, const float* in2,
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

    double overallscale = 1.0;
    overallscale /= 44100.0;
    overallscale *= getSampleRate();

    double compFThresh = std::pow (A, 4);
    double compFAttack = 1.0 / (((std::pow (B, 3) * 5000.0) + 500.0) * overallscale);
    double compFRelease = 1.0 / (((std::pow (C, 5) * 50000.0) + 500.0) * overallscale);
    double fireGain = D * 2.0; fireGain = std::pow (fireGain, 3);
    double firePad = fireGain; if (firePad > 1.0) firePad = 1.0;

    double compSThresh = std::pow (E, 4);
    double compSAttack = 1.0 / (((std::pow (F, 3) * 5000.0) + 500.0) * overallscale);
    double compSRelease = 1.0 / (((std::pow (G, 5) * 50000.0) + 500.0) * overallscale);
    double stoneGain = H * 2.0; stoneGain = std::pow (stoneGain, 3);
    double stonePad = stoneGain; if (stonePad > 1.0) stonePad = 1.0;

    double kalman = 1.0 - (std::pow (I, 2) / overallscale);
    //crossover frequency between mid/bass
    double compRatio = 1.0 - std::pow (1.0 - J, 2);

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;

        //begin KalmanL
        double fireL = inputSampleL;
        double temp = inputSampleL = inputSampleL * (1.0 - kalman) * 0.777;
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
        double stoneL = kal[kalOutL] * 0.777;
        fireL -= stoneL;
        //end KalmanL

        //begin KalmanR
        double fireR = inputSampleR;
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
        double stoneR = kal[kalOutR] * 0.777;
        fireR -= stoneR;
        //end KalmanR

        //fire dynamics
        if (std::fabs (fireL) > compFThresh)
        { //compression L
            fireCompL -= (fireCompL * compFAttack);
            fireCompL += ((compFThresh / std::fabs (fireL)) * compFAttack);
        }
        else fireCompL = (fireCompL * (1.0 - compFRelease)) + compFRelease;
        if (std::fabs (fireR) > compFThresh)
        { //compression R
            fireCompR -= (fireCompR * compFAttack);
            fireCompR += ((compFThresh / std::fabs (fireR)) * compFAttack);
        }
        else fireCompR = (fireCompR * (1.0 - compFRelease)) + compFRelease;
        if (fireCompL > fireCompR) fireCompL -= (fireCompL * compFAttack);
        if (fireCompR > fireCompL) fireCompR -= (fireCompR * compFAttack);
        fireCompL = std::max (std::min (fireCompL, 1.0), 0.0);
        fireCompR = std::max (std::min (fireCompR, 1.0), 0.0);
        fireL *= (((1.0 - compRatio) * firePad) + (fireCompL * compRatio * fireGain));
        fireR *= (((1.0 - compRatio) * firePad) + (fireCompR * compRatio * fireGain));

        //stone dynamics
        if (std::fabs (stoneL) > compSThresh)
        { //compression L
            stoneCompL -= (stoneCompL * compSAttack);
            stoneCompL += ((compSThresh / std::fabs (stoneL)) * compSAttack);
        }
        else stoneCompL = (stoneCompL * (1.0 - compSRelease)) + compSRelease;
        if (std::fabs (stoneR) > compSThresh)
        { //compression R
            stoneCompR -= (stoneCompR * compSAttack);
            stoneCompR += ((compSThresh / std::fabs (stoneR)) * compSAttack);
        }
        else stoneCompR = (stoneCompR * (1.0 - compSRelease)) + compSRelease;
        if (stoneCompL > stoneCompR) stoneCompL -= (stoneCompL * compSAttack);
        if (stoneCompR > stoneCompL) stoneCompR -= (stoneCompR * compSAttack);
        stoneCompL = std::max (std::min (stoneCompL, 1.0), 0.0);
        stoneCompR = std::max (std::min (stoneCompR, 1.0), 0.0);
        stoneL *= (((1.0 - compRatio) * stonePad) + (stoneCompL * compRatio * stoneGain));
        stoneR *= (((1.0 - compRatio) * stonePad) + (stoneCompR * compRatio * stoneGain));

        inputSampleL = stoneL + fireL;
        inputSampleR = stoneR + fireR;

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
