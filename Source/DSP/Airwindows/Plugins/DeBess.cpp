/* ========================================
 *  DeBess - DeBess.cpp
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/DeBess —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter
 *  (entspricht dem processDoubleReplacing-Pfad des Originals).
 * ======================================== */

#include "DSP/Airwindows/Plugins/DeBess.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "intense", "Intense", 0.0f },
        { "sharp",   "Sharp",   0.5f },
        { "depth",   "Depth",   0.5f },
        { "filter",  "Filter",  0.5f },
        { "sense",   "Sense",   0.0f },
    };
}

DeBess::DeBess() noexcept
    : AirwindowsPlugin ("debess", "DeBess", kParameters)
{
}

void DeBess::resetState() noexcept
{
    for (double& v : sL) v = 0.0;
    for (double& v : mL) v = 0.0;
    for (double& v : cL) v = 0.0;
    for (double& v : sR) v = 0.0;
    for (double& v : mR) v = 0.0;
    for (double& v : cR) v = 0.0;

    ratioAL = ratioBL = 1.0;
    iirSampleAL = 0.0;
    iirSampleBL = 0.0;
    ratioAR = ratioBR = 1.0;
    iirSampleAR = 0.0;
    iirSampleBR = 0.0;

    flip = false;
}

void DeBess::processStereo (const float* in1, const float* in2,
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

    double intensity = std::pow ((double) A, 5) * (8192.0 / overallscale);
    double sharpness = (double) B * 40.0;
    if (sharpness < 2) sharpness = 2;
    double speed = 0.1 / sharpness;
    double depth = 1.0 / ((double) C + 0.0001);
    double iirAmount = D;
    float monitoring = E;
    const int sharpInt = (int) sharpness; // 2..40, passt in die 41er-Arrays

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;

        sL[0] = inputSampleL; //set up so both [0] and [1] will be input sample
        sR[0] = inputSampleR; //set up so both [0] and [1] will be input sample
        //we only use the [1] so this is just where samples come in
        for (int x = sharpInt; x > 0; x--) {
            sL[x] = sL[x - 1];
            sR[x] = sR[x - 1];
        } //building up a set of slews

        mL[1] = (sL[1] - sL[2]) * ((sL[1] - sL[2]) / 1.3);
        mR[1] = (sR[1] - sR[2]) * ((sR[1] - sR[2]) / 1.3);
        for (int x = sharpInt - 1; x > 1; x--) {
            mL[x] = (sL[x] - sL[x + 1]) * ((sL[x - 1] - sL[x]) / 1.3);
            mR[x] = (sR[x] - sR[x + 1]) * ((sR[x - 1] - sR[x]) / 1.3);
        } //building up a set of slews of slews

        double senseL = std::fabs (mL[1] - mL[2]) * sharpness * sharpness;
        double senseR = std::fabs (mR[1] - mR[2]) * sharpness * sharpness;
        for (int x = sharpInt - 1; x > 0; x--) {
            double multL = std::fabs (mL[x] - mL[x + 1]) * sharpness * sharpness;
            if (multL < 1.0) senseL *= multL;
            double multR = std::fabs (mR[x] - mR[x + 1]) * sharpness * sharpness;
            if (multR < 1.0) senseR *= multR;
        } //sense is slews of slews times each other

        senseL = 1.0 + (intensity * intensity * senseL);
        if (senseL > intensity) { senseL = intensity; }
        senseR = 1.0 + (intensity * intensity * senseR);
        if (senseR > intensity) { senseR = intensity; }

        if (flip) {
            iirSampleAL = (iirSampleAL * (1 - iirAmount)) + (inputSampleL * iirAmount);
            iirSampleAR = (iirSampleAR * (1 - iirAmount)) + (inputSampleR * iirAmount);
            ratioAL = (ratioAL * (1.0 - speed)) + (senseL * speed);
            ratioAR = (ratioAR * (1.0 - speed)) + (senseR * speed);
            if (ratioAL > depth) ratioAL = depth;
            if (ratioAR > depth) ratioAR = depth;
            if (ratioAL > 1.0) inputSampleL = iirSampleAL + ((inputSampleL - iirSampleAL) / ratioAL);
            if (ratioAR > 1.0) inputSampleR = iirSampleAR + ((inputSampleR - iirSampleAR) / ratioAR);
        }
        else {
            iirSampleBL = (iirSampleBL * (1 - iirAmount)) + (inputSampleL * iirAmount);
            iirSampleBR = (iirSampleBR * (1 - iirAmount)) + (inputSampleR * iirAmount);
            ratioBL = (ratioBL * (1.0 - speed)) + (senseL * speed);
            ratioBR = (ratioBR * (1.0 - speed)) + (senseR * speed);
            if (ratioBL > depth) ratioBL = depth;
            if (ratioBR > depth) ratioBR = depth;
            if (ratioAL > 1.0) inputSampleL = iirSampleBL + ((inputSampleL - iirSampleBL) / ratioBL);
            if (ratioAR > 1.0) inputSampleR = iirSampleBR + ((inputSampleR - iirSampleBR) / ratioBR);
        }
        flip = !flip;

        if (monitoring > 0.49999f) {
            inputSampleL = *in1 - inputSampleL;
            inputSampleR = *in2 - inputSampleR;
        }
        //sense monitoring

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
