/* ========================================
 *  FatEQ - FatEQ.cpp
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/FatEQ —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter.
 * ======================================== */

#include "DSP/Airwindows/Plugins/FatEQ.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "high", "High", 0.5f },
        { "hmid", "HMid", 0.5f },
        { "lmid", "LMid", 0.5f },
        { "bass", "Bass", 0.5f },
        { "out",  "Out",  1.0f },
    };

    constexpr double kHalfPi = juce::MathConstants<double>::halfPi;
}

FatEQ::FatEQ() noexcept
    : AirwindowsPlugin ("fateq", "FatEQ", kParameters)
{
}

void FatEQ::resetState() noexcept
{
    for (double& v : pearA) v = 0.0;
    for (double& v : pearB) v = 0.0;
    for (double& v : pearC) v = 0.0;

    lastSampleL = 0.0;
    wasPosClipL = false;
    wasNegClipL = false;
    lastSampleR = 0.0;
    wasPosClipR = false;
    wasNegClipR = false;
    for (double& v : intermediateL) v = 0.0;
    for (double& v : intermediateR) v = 0.0;
}

void FatEQ::processStereo (const float* in1, const float* in2,
                          float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);
    const float B = param (kParamB);
    const float C = param (kParamC);
    const float D = param (kParamD);
    const float E = param (kParamE);

    double overallscale = 1.0;
    overallscale /= 44100.0;
    overallscale *= getSampleRate();
    int spacing = (int) std::floor (overallscale); //should give us working basic scaling, usually 2 or 4
    if (spacing < 1) spacing = 1; if (spacing > 16) spacing = 16;

    double topLevl = A * 2.0; if (topLevl < 1.0) topLevl = std::sqrt (topLevl);
    pearA[levl] = B * 2.0; if (pearA[levl] < 1.0) pearA[levl] = std::sqrt (pearA[levl]);
    pearB[levl] = C * 2.0; if (pearB[levl] < 1.0) pearB[levl] = std::sqrt (pearB[levl]);
    pearC[levl] = D * 2.0; if (pearC[levl] < 1.0) pearC[levl] = std::sqrt (pearC[levl]);
    double out = E;

    double freqFactor = std::sqrt (overallscale) + (overallscale * 0.5);
    pearA[freq] = std::pow (0.564, freqFactor + 0.85);
    pearB[freq] = std::pow (0.564, freqFactor + 4.1);
    pearC[freq] = std::pow (0.564, freqFactor + 7.1);

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;

        for (int x = 0; x < pear_max; x += 4)
        {
            //begin Pear filter stages
            pearA[figL] = inputSampleL; pearA[figR] = inputSampleR;
            pearA[slew] = ((pearA[figL] - pearA[x]) + pearA[x + 1]) * pearA[freq] * 0.5;
            pearA[x] = pearA[figL] = (pearA[freq] * pearA[figL]) + ((1.0 - pearA[freq]) * (pearA[x] + pearA[x + 1]));
            pearA[x + 1] = pearA[slew];
            pearA[slew] = ((pearA[figR] - pearA[x + 2]) + pearA[x + 3]) * pearA[freq] * 0.5;
            pearA[x + 2] = pearA[figR] = (pearA[freq] * pearA[figR]) + ((1.0 - pearA[freq]) * (pearA[x + 2] + pearA[x + 3]));
            pearA[x + 3] = pearA[slew];
            inputSampleL -= pearA[figL]; inputSampleR -= pearA[figR];

            pearB[figL] = pearA[figL]; pearB[figR] = pearA[figR];
            pearB[slew] = ((pearB[figL] - pearB[x]) + pearB[x + 1]) * pearB[freq] * 0.5;
            pearB[x] = pearB[figL] = (pearB[freq] * pearA[figL]) + ((1.0 - pearB[freq]) * (pearB[x] + pearB[x + 1]));
            pearB[x + 1] = pearB[slew];
            pearB[slew] = ((pearB[figR] - pearB[x + 2]) + pearB[x + 3]) * pearB[freq] * 0.5;
            pearB[x + 2] = pearB[figR] = (pearB[freq] * pearA[figR]) + ((1.0 - pearB[freq]) * (pearB[x + 2] + pearB[x + 3]));
            pearB[x + 3] = pearB[slew];
            pearA[figL] -= pearB[figL]; pearA[figR] -= pearB[figR];

            pearC[figL] = pearB[figL]; pearC[figR] = pearB[figR];
            pearC[slew] = ((pearC[figL] - pearC[x]) + pearC[x + 1]) * pearC[freq] * 0.5;
            pearC[x] = pearC[figL] = (pearC[freq] * pearB[figL]) + ((1.0 - pearC[freq]) * (pearC[x] + pearC[x + 1]));
            pearC[x + 1] = pearC[slew];
            pearC[slew] = ((pearC[figR] - pearC[x + 2]) + pearC[x + 3]) * pearC[freq] * 0.5;
            pearC[x + 2] = pearC[figR] = (pearC[freq] * pearB[figR]) + ((1.0 - pearC[freq]) * (pearC[x + 2] + pearC[x + 3]));
            pearC[x + 3] = pearC[slew];
            pearB[figL] -= pearC[figL]; pearB[figR] -= pearC[figR];

            //begin FatEQL
            double altered = inputSampleL;
            if (topLevl > 1.0)
            {
                altered = std::fmax (std::fmin (inputSampleL * kHalfPi, kHalfPi), -kHalfPi);
                double X = altered * altered;
                double temp = altered * X; altered -= (temp / 6.0); temp *= X;
                altered += (temp / 120.0); temp *= X; altered -= (temp / 5040.0); temp *= X;
                altered += (temp / 362880.0); temp *= X; altered -= (temp / 39916800.0);
            }
            if (topLevl < 1.0)
            {
                altered = std::fmax (std::fmin (inputSampleL, 1.0), -1.0);
                double polarity = altered;
                double X = inputSampleL * altered;
                double temp = X; altered = (temp / 2.0); temp *= X;
                altered -= (temp / 24.0); temp *= X; altered += (temp / 720.0); temp *= X;
                altered -= (temp / 40320.0); temp *= X; altered += (temp / 3628800.0);
                altered *= ((polarity < 0.0) ? -1.0 : 1.0);
            }
            inputSampleL = (inputSampleL * (1.0 - std::fabs (topLevl - 1.0))) + (altered * std::fabs (topLevl - 1.0));

            altered = pearA[figL];
            if (pearA[levl] > 1.0)
            {
                altered = std::fmax (std::fmin (pearA[figL] * kHalfPi, kHalfPi), -kHalfPi);
                double X = altered * altered;
                double temp = altered * X; altered -= (temp / 6.0); temp *= X;
                altered += (temp / 120.0); temp *= X; altered -= (temp / 5040.0); temp *= X;
                altered += (temp / 362880.0); temp *= X; altered -= (temp / 39916800.0);
            }
            if (pearA[levl] < 1.0)
            {
                altered = std::fmax (std::fmin (pearA[figL], 1.0), -1.0);
                double polarity = altered;
                double X = pearA[figL] * altered;
                double temp = X; altered = (temp / 2.0); temp *= X;
                altered -= (temp / 24.0); temp *= X; altered += (temp / 720.0); temp *= X;
                altered -= (temp / 40320.0); temp *= X; altered += (temp / 3628800.0);
                altered *= ((polarity < 0.0) ? -1.0 : 1.0);
            }
            inputSampleL += (pearA[figL] * (1.0 - std::fabs (pearA[levl] - 1.0))) + (altered * std::fabs (pearA[levl] - 1.0));

            altered = pearB[figL];
            if (pearB[levl] > 1.0)
            {
                altered = std::fmax (std::fmin (pearB[figL] * kHalfPi, kHalfPi), -kHalfPi);
                double X = altered * altered;
                double temp = altered * X; altered -= (temp / 6.0); temp *= X;
                altered += (temp / 120.0); temp *= X; altered -= (temp / 5040.0); temp *= X;
                altered += (temp / 362880.0); temp *= X; altered -= (temp / 39916800.0);
            }
            if (pearB[levl] < 1.0)
            {
                altered = std::fmax (std::fmin (pearB[figL], 1.0), -1.0);
                double polarity = altered;
                double X = pearB[figL] * altered;
                double temp = X; altered = (temp / 2.0); temp *= X;
                altered -= (temp / 24.0); temp *= X; altered += (temp / 720.0); temp *= X;
                altered -= (temp / 40320.0); temp *= X; altered += (temp / 3628800.0);
                altered *= ((polarity < 0.0) ? -1.0 : 1.0);
            }
            inputSampleL += (pearB[figL] * (1.0 - std::fabs (pearB[levl] - 1.0))) + (altered * std::fabs (pearB[levl] - 1.0));

            altered = pearC[figL];
            if (pearC[levl] > 1.0)
            {
                altered = std::fmax (std::fmin (pearC[figL] * kHalfPi, kHalfPi), -kHalfPi);
                double X = altered * altered;
                double temp = altered * X; altered -= (temp / 6.0); temp *= X;
                altered += (temp / 120.0); temp *= X; altered -= (temp / 5040.0); temp *= X;
                altered += (temp / 362880.0); temp *= X; altered -= (temp / 39916800.0);
            }
            if (pearC[levl] < 1.0)
            {
                altered = std::fmax (std::fmin (pearC[figL], 1.0), -1.0);
                double polarity = altered;
                double X = pearC[figL] * altered;
                double temp = X; altered = (temp / 2.0); temp *= X;
                altered -= (temp / 24.0); temp *= X; altered += (temp / 720.0); temp *= X;
                altered -= (temp / 40320.0); temp *= X; altered += (temp / 3628800.0);
                altered *= ((polarity < 0.0) ? -1.0 : 1.0);
            }
            inputSampleL += (pearC[figL] * (1.0 - std::fabs (pearC[levl] - 1.0))) + (altered * std::fabs (pearC[levl] - 1.0));
            //end FatEQ L

            //begin FatEQ R
            altered = inputSampleR;
            if (topLevl > 1.0)
            {
                altered = std::fmax (std::fmin (inputSampleR * kHalfPi, kHalfPi), -kHalfPi);
                double X = altered * altered;
                double temp = altered * X; altered -= (temp / 6.0); temp *= X;
                altered += (temp / 120.0); temp *= X; altered -= (temp / 5040.0); temp *= X;
                altered += (temp / 362880.0); temp *= X; altered -= (temp / 39916800.0);
            }
            if (topLevl < 1.0)
            {
                altered = std::fmax (std::fmin (inputSampleR, 1.0), -1.0);
                double polarity = altered;
                double X = inputSampleR * altered;
                double temp = X; altered = (temp / 2.0); temp *= X;
                altered -= (temp / 24.0); temp *= X; altered += (temp / 720.0); temp *= X;
                altered -= (temp / 40320.0); temp *= X; altered += (temp / 3628800.0);
                altered *= ((polarity < 0.0) ? -1.0 : 1.0);
            }
            inputSampleR = (inputSampleR * (1.0 - std::fabs (topLevl - 1.0))) + (altered * std::fabs (topLevl - 1.0));

            altered = pearA[figR];
            if (pearA[levl] > 1.0)
            {
                altered = std::fmax (std::fmin (pearA[figR] * kHalfPi, kHalfPi), -kHalfPi);
                double X = altered * altered;
                double temp = altered * X; altered -= (temp / 6.0); temp *= X;
                altered += (temp / 120.0); temp *= X; altered -= (temp / 5040.0); temp *= X;
                altered += (temp / 362880.0); temp *= X; altered -= (temp / 39916800.0);
            }
            if (pearA[levl] < 1.0)
            {
                altered = std::fmax (std::fmin (pearA[figR], 1.0), -1.0);
                double polarity = altered;
                double X = pearA[figR] * altered;
                double temp = X; altered = (temp / 2.0); temp *= X;
                altered -= (temp / 24.0); temp *= X; altered += (temp / 720.0); temp *= X;
                altered -= (temp / 40320.0); temp *= X; altered += (temp / 3628800.0);
                altered *= ((polarity < 0.0) ? -1.0 : 1.0);
            }
            inputSampleR += (pearA[figR] * (1.0 - std::fabs (pearA[levl] - 1.0))) + (altered * std::fabs (pearA[levl] - 1.0));

            altered = pearB[figR];
            if (pearB[levl] > 1.0)
            {
                altered = std::fmax (std::fmin (pearB[figR] * kHalfPi, kHalfPi), -kHalfPi);
                double X = altered * altered;
                double temp = altered * X; altered -= (temp / 6.0); temp *= X;
                altered += (temp / 120.0); temp *= X; altered -= (temp / 5040.0); temp *= X;
                altered += (temp / 362880.0); temp *= X; altered -= (temp / 39916800.0);
            }
            if (pearB[levl] < 1.0)
            {
                altered = std::fmax (std::fmin (pearB[figR], 1.0), -1.0);
                double polarity = altered;
                double X = pearB[figR] * altered;
                double temp = X; altered = (temp / 2.0); temp *= X;
                altered -= (temp / 24.0); temp *= X; altered += (temp / 720.0); temp *= X;
                altered -= (temp / 40320.0); temp *= X; altered += (temp / 3628800.0);
                altered *= ((polarity < 0.0) ? -1.0 : 1.0);
            }
            inputSampleR += (pearB[figR] * (1.0 - std::fabs (pearB[levl] - 1.0))) + (altered * std::fabs (pearB[levl] - 1.0));

            altered = pearC[figR];
            if (pearC[levl] > 1.0)
            {
                altered = std::fmax (std::fmin (pearC[figR] * kHalfPi, kHalfPi), -kHalfPi);
                double X = altered * altered;
                double temp = altered * X; altered -= (temp / 6.0); temp *= X;
                altered += (temp / 120.0); temp *= X; altered -= (temp / 5040.0); temp *= X;
                altered += (temp / 362880.0); temp *= X; altered -= (temp / 39916800.0);
            }
            if (pearC[levl] < 1.0)
            {
                altered = std::fmax (std::fmin (pearC[figR], 1.0), -1.0);
                double polarity = altered;
                double X = pearC[figR] * altered;
                double temp = X; altered = (temp / 2.0); temp *= X;
                altered -= (temp / 24.0); temp *= X; altered += (temp / 720.0); temp *= X;
                altered -= (temp / 40320.0); temp *= X; altered += (temp / 3628800.0);
                altered *= ((polarity < 0.0) ? -1.0 : 1.0);
            }
            inputSampleR += (pearC[figR] * (1.0 - std::fabs (pearC[levl] - 1.0))) + (altered * std::fabs (pearC[levl] - 1.0));
            //end FatEQ R
        }

        inputSampleL *= out;
        inputSampleR *= out;

        //begin ClipOnly2 stereo as a little, compressed chunk that can be dropped into code
        if (inputSampleL > 4.0) inputSampleL = 4.0; if (inputSampleL < -4.0) inputSampleL = -4.0;
        if (wasPosClipL == true)
        { //current will be over
            if (inputSampleL < lastSampleL) lastSampleL = 0.7058208 + (inputSampleL * 0.2609148);
            else lastSampleL = 0.2491717 + (lastSampleL * 0.7390851);
        } wasPosClipL = false;
        if (inputSampleL > 0.9549925859) { wasPosClipL = true; inputSampleL = 0.7058208 + (lastSampleL * 0.2609148); }
        if (wasNegClipL == true)
        { //current will be -over
            if (inputSampleL > lastSampleL) lastSampleL = -0.7058208 + (inputSampleL * 0.2609148);
            else lastSampleL = -0.2491717 + (lastSampleL * 0.7390851);
        } wasNegClipL = false;
        if (inputSampleL < -0.9549925859) { wasNegClipL = true; inputSampleL = -0.7058208 + (lastSampleL * 0.2609148); }
        intermediateL[spacing] = inputSampleL;
        inputSampleL = lastSampleL; //Latency is however many samples equals one 44.1k sample
        for (int x = spacing; x > 0; x--) intermediateL[x - 1] = intermediateL[x];
        lastSampleL = intermediateL[0]; //run a little buffer to handle this

        if (inputSampleR > 4.0) inputSampleR = 4.0; if (inputSampleR < -4.0) inputSampleR = -4.0;
        if (wasPosClipR == true)
        { //current will be over
            if (inputSampleR < lastSampleR) lastSampleR = 0.7058208 + (inputSampleR * 0.2609148);
            else lastSampleR = 0.2491717 + (lastSampleR * 0.7390851);
        } wasPosClipR = false;
        if (inputSampleR > 0.9549925859) { wasPosClipR = true; inputSampleR = 0.7058208 + (lastSampleR * 0.2609148); }
        if (wasNegClipR == true)
        { //current will be -over
            if (inputSampleR > lastSampleR) lastSampleR = -0.7058208 + (inputSampleR * 0.2609148);
            else lastSampleR = -0.2491717 + (lastSampleR * 0.7390851);
        } wasNegClipR = false;
        if (inputSampleR < -0.9549925859) { wasNegClipR = true; inputSampleR = -0.7058208 + (lastSampleR * 0.2609148); }
        intermediateR[spacing] = inputSampleR;
        inputSampleR = lastSampleR; //Latency is however many samples equals one 44.1k sample
        for (int x = spacing; x > 0; x--) intermediateR[x - 1] = intermediateR[x];
        lastSampleR = intermediateR[0]; //run a little buffer to handle this
        //end ClipOnly2 stereo as a little, compressed chunk that can be dropped into code

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
