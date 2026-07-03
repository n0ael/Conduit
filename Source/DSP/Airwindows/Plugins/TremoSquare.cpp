/* ========================================
 *  TremoSquare - TremoSquare.cpp
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/TremoSquare —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter
 *  (entspricht dem processDoubleReplacing-Pfad des Originals).
 * ======================================== */

#include "DSP/Airwindows/Plugins/TremoSquare.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "freq",    "Freq",    0.5f },
        { "dry_wet", "Dry/Wet", 1.0f },
    };
}

TremoSquare::TremoSquare() noexcept
    : AirwindowsPlugin ("tremosquare", "TremoSquare", kParameters)
{
}

void TremoSquare::resetState() noexcept
{
    osc = 0.0;
    polarityL = false;
    muteL = false;
    polarityR = false;
    muteR = false;
}

void TremoSquare::processStereo (const float* in1, const float* in2,
                                 float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);
    const float B = param (kParamB);

    double overallscale = 1.0;
    overallscale /= 44100.0;
    overallscale *= getSampleRate();

    double increment = std::pow (A, 4) / (50.0 * overallscale);
    double wet = B;

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;
        double drySampleL = inputSampleL;
        double drySampleR = inputSampleR;

        osc += increment; if (osc > 1.0) osc = 0.0;
        // this is our little oscillator code

        if (inputSampleL < 0)
        {
            if (polarityL == true) muteL = (osc < 0.5);
            polarityL = false;
        }
        else
        {
            if (polarityL == false) muteL = (osc < 0.5);
            polarityL = true;
        }

        if (inputSampleR < 0)
        {
            if (polarityR == true) muteR = (osc < 0.5);
            polarityR = false;
        }
        else
        {
            if (polarityR == false) muteR = (osc < 0.5);
            polarityR = true;
        }

        if (muteL) inputSampleL = 0.0;
        if (muteR) inputSampleR = 0.0;

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
