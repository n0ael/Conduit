/* ========================================
 *  Pockey2 - Pockey2.cpp
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Pockey2 —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter
 *  (entspricht dem processDoubleReplacing-Pfad des Originals).
 * ======================================== */

#include "DSP/Airwindows/Plugins/Pockey2.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "de_freq", "DeFreq",  0.0f },
        { "de_rez",  "DeRez",   0.66f },
        { "dry_wet", "Dry/Wet", 1.0f },
    };
}

Pockey2::Pockey2() noexcept
    : AirwindowsPlugin ("pockey2", "Pockey2", kParameters)
{
}

void Pockey2::resetState() noexcept
{
    lastSampleL = 0.0;
    heldSampleL = 0.0;
    previousHeldL = 0.0;

    lastSampleR = 0.0;
    heldSampleR = 0.0;
    previousHeldR = 0.0;

    position = 0.0;
}

void Pockey2::processStereo (const float* in1, const float* in2,
                            float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);
    const float B = param (kParamB);
    const float C = param (kParamC);

    double overallscale = 1.0;
    overallscale /= 44100.0;
    overallscale *= getSampleRate();

    int freq = (int) std::floor (std::pow (A, 3) * 32.0 * overallscale);
    //dividing of derez must always be integer values now: no freq grinding

    double rez = 4 + (B * 12.0);
    //4 to 16, with 12 being the default.
    int rezFactor = (int) std::pow (2, rez); //256, 4096, 65536 or anything in between

    double wet = C;

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;
        double drySampleL = inputSampleL;
        double drySampleR = inputSampleR;

        if (inputSampleL > 1.0) inputSampleL = 1.0; if (inputSampleL < -1.0) inputSampleL = -1.0;
        if (inputSampleL > 0) inputSampleL = std::log (1.0 + (255 * std::fabs (inputSampleL))) / std::log (255);
        if (inputSampleL < 0) inputSampleL = -std::log (1.0 + (255 * std::fabs (inputSampleL))) / std::log (255);

        if (inputSampleR > 1.0) inputSampleR = 1.0; if (inputSampleR < -1.0) inputSampleR = -1.0;
        if (inputSampleR > 0) inputSampleR = std::log (1.0 + (255 * std::fabs (inputSampleR))) / std::log (255);
        if (inputSampleR < 0) inputSampleR = -std::log (1.0 + (255 * std::fabs (inputSampleR))) / std::log (255);
        //end uLaw encode

        inputSampleL *= rezFactor;
        inputSampleR *= rezFactor;
        if (inputSampleL > 0) inputSampleL = std::floor (inputSampleL);
        if (inputSampleL < 0) inputSampleL = -std::floor (-inputSampleL);
        if (inputSampleR > 0) inputSampleR = std::floor (inputSampleR);
        if (inputSampleR < 0) inputSampleR = -std::floor (-inputSampleR);
        inputSampleL /= rezFactor;
        inputSampleR /= rezFactor;

        if (inputSampleL > 1.0) inputSampleL = 1.0; if (inputSampleL < -1.0) inputSampleL = -1.0;
        if (inputSampleL > 0) inputSampleL = (std::pow (256, std::fabs (inputSampleL)) - 1.0) / 255;
        if (inputSampleL < 0) inputSampleL = -(std::pow (256, std::fabs (inputSampleL)) - 1.0) / 255;

        if (inputSampleR > 1.0) inputSampleR = 1.0; if (inputSampleR < -1.0) inputSampleR = -1.0;
        if (inputSampleR > 0) inputSampleR = (std::pow (256, std::fabs (inputSampleR)) - 1.0) / 255;
        if (inputSampleR < 0) inputSampleR = -(std::pow (256, std::fabs (inputSampleR)) - 1.0) / 255;
        //end uLaw decode

        double blurL = 0.618033988749894848204586 - (std::fabs (inputSampleL - lastSampleL) * overallscale);
        if (blurL < 0.0) blurL = 0.0;
        double blurR = 0.618033988749894848204586 - (std::fabs (inputSampleR - lastSampleR) * overallscale);
        if (blurR < 0.0) blurR = 0.0; //reverse it. Mellow stuff gets blur, bright gets edge

        if (position < 1)
        {
            position = freq; //one to ? scaled by overallscale
            heldSampleL = inputSampleL;
            heldSampleR = inputSampleR;
        }
        inputSampleL = heldSampleL;
        inputSampleR = heldSampleR;
        lastSampleL = drySampleL;
        lastSampleR = drySampleR;
        position--;

        inputSampleL = (inputSampleL * blurL) + (previousHeldL * (1.0 - blurL));
        inputSampleR = (inputSampleR * blurR) + (previousHeldR * (1.0 - blurR));
        //conditional average: only if we actually have brightness
        previousHeldL = heldSampleL;
        previousHeldR = heldSampleR;
        //end Frequency Derez

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
