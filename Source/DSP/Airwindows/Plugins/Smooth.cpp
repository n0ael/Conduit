/* ========================================
 *  Smooth - Smooth.cpp
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Smooth —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter
 *  (entspricht dem processDoubleReplacing-Pfad des Originals).
 * ======================================== */

#include "DSP/Airwindows/Plugins/Smooth.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "smooth",  "Smooth",  0.0f },
        { "output",  "Output",  1.0f },
        { "dry_wet", "Dry/Wet", 1.0f },
    };
}

Smooth::Smooth() noexcept
    : AirwindowsPlugin ("smooth", "Smooth", kParameters)
{
}

void Smooth::resetState() noexcept
{
    lastSampleAL = 0.0; gainAL = 1.0;
    lastSampleBL = 0.0; gainBL = 1.0;
    lastSampleCL = 0.0; gainCL = 1.0;
    lastSampleDL = 0.0; gainDL = 1.0;
    lastSampleEL = 0.0; gainEL = 1.0;

    lastSampleAR = 0.0; gainAR = 1.0;
    lastSampleBR = 0.0; gainBR = 1.0;
    lastSampleCR = 0.0; gainCR = 1.0;
    lastSampleDR = 0.0; gainDR = 1.0;
    lastSampleER = 0.0; gainER = 1.0;
}

void Smooth::processStereo (const float* in1, const float* in2,
                           float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);
    const float B = param (kParamB);
    const float C = param (kParamC);

    double overallscale = 1.0;
    overallscale /= 44100.0;
    overallscale *= getSampleRate();

    double clamp;
    double chase = std::pow (A, 2);
    double makeup = (1.0 + (chase * 1.6)) * B;
    double ratio = chase * 24.0;
    chase /= overallscale;
    chase *= 0.083; // set up the ratio of new val to old
    double wet = C;

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;

        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;
        double drySampleL = inputSampleL;
        double drySampleR = inputSampleR;

        //left channel
        clamp = std::fabs (inputSampleL - lastSampleAL);
        clamp = std::sin (clamp * ratio);
        lastSampleAL = inputSampleL;
        gainAL *= (1.0 - chase);
        gainAL += ((1.0 - clamp) * chase);
        if (gainAL > 1.0) gainAL = 1.0;
        if (gainAL < 0.0) gainAL = 0.0;
        if (gainAL != 1.0) inputSampleL *= gainAL;

        clamp = std::fabs (inputSampleL - lastSampleBL);
        clamp = std::sin (clamp * ratio);
        lastSampleBL = inputSampleL;
        gainBL *= (1.0 - chase);
        gainBL += ((1.0 - clamp) * chase);
        if (gainBL > 1.0) gainBL = 1.0;
        if (gainBL < 0.0) gainBL = 0.0;
        if (gainBL != 1.0) inputSampleL *= gainBL;

        clamp = std::fabs (inputSampleL - lastSampleCL);
        clamp = std::sin (clamp * ratio);
        lastSampleCL = inputSampleL;
        gainCL *= (1.0 - chase);
        gainCL += ((1.0 - clamp) * chase);
        if (gainCL > 1.0) gainCL = 1.0;
        if (gainCL < 0.0) gainCL = 0.0;
        if (gainCL != 1.0) inputSampleL *= gainCL;

        clamp = std::fabs (inputSampleL - lastSampleDL);
        clamp = std::sin (clamp * ratio);
        lastSampleDL = inputSampleL;
        gainDL *= (1.0 - chase);
        gainDL += ((1.0 - clamp) * chase);
        if (gainDL > 1.0) gainDL = 1.0;
        if (gainDL < 0.0) gainDL = 0.0;
        if (gainDL != 1.0) inputSampleL *= gainDL;

        clamp = std::fabs (inputSampleL - lastSampleEL);
        clamp = std::sin (clamp * ratio);
        lastSampleEL = inputSampleL;
        gainEL *= (1.0 - chase);
        gainEL += ((1.0 - clamp) * chase);
        if (gainEL > 1.0) gainEL = 1.0;
        if (gainEL < 0.0) gainEL = 0.0;
        if (gainEL != 1.0) inputSampleL *= gainEL;
        //end left channel

        //right channel
        clamp = std::fabs (inputSampleR - lastSampleAR);
        clamp = std::sin (clamp * ratio);
        lastSampleAR = inputSampleR;
        gainAR *= (1.0 - chase);
        gainAR += ((1.0 - clamp) * chase);
        if (gainAR > 1.0) gainAR = 1.0;
        if (gainAR < 0.0) gainAR = 0.0;
        if (gainAR != 1.0) inputSampleR *= gainAR;

        clamp = std::fabs (inputSampleR - lastSampleBR);
        clamp = std::sin (clamp * ratio);
        lastSampleBR = inputSampleR;
        gainBR *= (1.0 - chase);
        gainBR += ((1.0 - clamp) * chase);
        if (gainBR > 1.0) gainBR = 1.0;
        if (gainBR < 0.0) gainBR = 0.0;
        if (gainBR != 1.0) inputSampleR *= gainBR;

        clamp = std::fabs (inputSampleR - lastSampleCR);
        clamp = std::sin (clamp * ratio);
        lastSampleCR = inputSampleR;
        gainCR *= (1.0 - chase);
        gainCR += ((1.0 - clamp) * chase);
        if (gainCR > 1.0) gainCR = 1.0;
        if (gainCR < 0.0) gainCR = 0.0;
        if (gainCR != 1.0) inputSampleR *= gainCR;

        clamp = std::fabs (inputSampleR - lastSampleDR);
        clamp = std::sin (clamp * ratio);
        lastSampleDR = inputSampleR;
        gainDR *= (1.0 - chase);
        gainDR += ((1.0 - clamp) * chase);
        if (gainDR > 1.0) gainDR = 1.0;
        if (gainDR < 0.0) gainDR = 0.0;
        if (gainDR != 1.0) inputSampleR *= gainDR;

        clamp = std::fabs (inputSampleR - lastSampleER);
        clamp = std::sin (clamp * ratio);
        lastSampleER = inputSampleR;
        gainER *= (1.0 - chase);
        gainER += ((1.0 - clamp) * chase);
        if (gainER > 1.0) gainER = 1.0;
        if (gainER < 0.0) gainER = 0.0;
        if (gainER != 1.0) inputSampleR *= gainER;
        //end right channel

        if (makeup != 1.0)
        {
            inputSampleL *= makeup;
            inputSampleR *= makeup;
        }

        if (wet != 1.0)
        {
            inputSampleL = (inputSampleL * wet) + (drySampleL * (1.0 - wet));
            inputSampleR = (inputSampleR * wet) + (drySampleR * (1.0 - wet));
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
