/* ========================================
 *  Trianglizer - Trianglizer.cpp
 *  Copyright (c) Airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Trianglizer —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter
 *  (entspricht dem processDoubleReplacing-Pfad des Originals).
 * ======================================== */

#include "DSP/Airwindows/Plugins/Trianglizer.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "tri_fat", "Tri/Fat", 0.0f },
        { "dry_wet", "Dry/Wet", 1.0f },
    };
}

Trianglizer::Trianglizer() noexcept
    : AirwindowsPlugin ("trianglizer", "Trianglizer", kParameters)
{
}

void Trianglizer::resetState() noexcept
{
    // zustandslos — nur fpdL/fpdR, die setzt die Basis in prepare()
}

void Trianglizer::processStereo (const float* in1, const float* in2,
                                 float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);
    const float B = param (kParamB);

    double offset = (A + 1.0) * 0.5;
    double inverter = (offset * 2.0) - 1.0;
    double wet = B;

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;
        double drySampleL = inputSampleL;
        double drySampleR = inputSampleR;

        if (inputSampleL > 1.0) inputSampleL = 1.0;
        else if (inputSampleL > 0.0) inputSampleL = -std::expm1 ((std::log1p (-inputSampleL) * (offset + (inputSampleL * inverter))));
        if (inputSampleL < -1.0) inputSampleL = -1.0;
        else if (inputSampleL < 0.0) inputSampleL = std::expm1 ((std::log1p (inputSampleL) * (offset - (inputSampleL * inverter))));

        if (inputSampleR > 1.0) inputSampleR = 1.0;
        else if (inputSampleR > 0.0) inputSampleR = -std::expm1 ((std::log1p (-inputSampleR) * (offset + (inputSampleR * inverter))));
        if (inputSampleR < -1.0) inputSampleR = -1.0;
        else if (inputSampleR < 0.0) inputSampleR = std::expm1 ((std::log1p (inputSampleR) * (offset - (inputSampleR * inverter))));

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
