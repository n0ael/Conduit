/* ========================================
 *  DigitalBlack - DigitalBlack.cpp
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/DigitalBlack —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter.
 * ======================================== */

#include "DSP/Airwindows/Plugins/DigitalBlack.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "thresh",  "Thresh",  0.0f },
        { "dry_wet", "Dry/Wet", 1.0f },
    };
}

DigitalBlack::DigitalBlack() noexcept
    : AirwindowsPlugin ("digitalblack", "DigitalBlack", kParameters)
{
}

void DigitalBlack::resetState() noexcept
{
    WasNegativeL = false;
    ZeroCrossL = 0;
    gaterollerL = 0.0;
    WasNegativeR = false;
    ZeroCrossR = 0;
    gaterollerR = 0.0;
}

void DigitalBlack::processStereo (const float* in1, const float* in2,
                                  float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);
    const float B = param (kParamB);

    double overallscale = 1.0;
    overallscale /= 44100.0;
    overallscale *= getSampleRate();

    double threshold = (std::pow (A, 4) / 3) + 0.00018;
    double release = 0.0064 / overallscale;
    double wet = B;

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;
        double drySampleL = inputSampleL;
        double drySampleR = inputSampleR;

        if (inputSampleL > 0)
        {
            if (WasNegativeL == true) ZeroCrossL = 0;
            WasNegativeL = false;
        }
        else
        {
            ZeroCrossL += 1;
            WasNegativeL = true;
        }
        if (inputSampleR > 0)
        {
            if (WasNegativeR == true) ZeroCrossR = 0;
            WasNegativeR = false;
        }
        else
        {
            ZeroCrossR += 1;
            WasNegativeR = true;
        }

        if (ZeroCrossL > 6000) ZeroCrossL = 6000;
        if (ZeroCrossR > 6000) ZeroCrossR = 6000;

        if (std::fabs (inputSampleL) > threshold)
        {
            if (gaterollerL < ZeroCrossL) gaterollerL = ZeroCrossL;
        }
        else
        {
            gaterollerL -= release;
        }
        if (std::fabs (inputSampleR) > threshold)
        {
            if (gaterollerR < ZeroCrossR) gaterollerR = ZeroCrossR;
        }
        else
        {
            gaterollerR -= release;
        }

        if (gaterollerL < 0.0) gaterollerL = 0.0;
        if (gaterollerR < 0.0) gaterollerR = 0.0;

        double gate = 1.0;
        if (gaterollerL < 1.0) gate = gaterollerL;

        double bridgerectifier = 1 - std::cos (std::fabs (inputSampleL));

        if (inputSampleL > 0) inputSampleL = (inputSampleL * gate) + (bridgerectifier * (1 - gate));
        else inputSampleL = (inputSampleL * gate) - (bridgerectifier * (1 - gate));

        if ((gate == 0.0) && (std::fabs (inputSampleL * 3) < threshold)) inputSampleL = 0.0;

        gate = 1.0;
        if (gaterollerR < 1.0) gate = gaterollerR;

        bridgerectifier = 1 - std::cos (std::fabs (inputSampleR));

        if (inputSampleR > 0) inputSampleR = (inputSampleR * gate) + (bridgerectifier * (1 - gate));
        else inputSampleR = (inputSampleR * gate) - (bridgerectifier * (1 - gate));

        if ((gate == 0.0) && (std::fabs (inputSampleR * 3) < threshold)) inputSampleR = 0.0;

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
