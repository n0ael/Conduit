/* ========================================
 *  Sweeten - Sweeten.cpp
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Sweeten —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter
 *  (entspricht dem processDoubleReplacing-Pfad des Originals).
 * ======================================== */

#include "DSP/Airwindows/Plugins/Sweeten.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "sweeten", "Sweeten", 0.0f },
    };
}

Sweeten::Sweeten() noexcept
    : AirwindowsPlugin ("sweeten", "Sweeten", kParameters)
{
}

void Sweeten::resetState() noexcept
{
    for (auto& v : savg) v = 0.0;
}

void Sweeten::processStereo (const float* in1, const float* in2,
                             float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);

    double overallscale = 1.0;
    overallscale /= 44100.0;
    overallscale *= getSampleRate();
    int cycleEnd = (int) std::floor (overallscale);
    if (cycleEnd < 1) cycleEnd = 1;
    if (cycleEnd > 4) cycleEnd = 4;
    //this is going to be 2 for 88.1 or 96k, 3 for silly people, 4 for 176 or 192k

    int sweetBits = 10 - (int) std::floor (A * 10.0);
    double sweet = 1.0;
    switch (sweetBits)
    {
        case 11: sweet = 0.00048828125; break;
        case 10: sweet = 0.0009765625; break;
        case 9: sweet = 0.001953125; break;
        case 8: sweet = 0.00390625; break;
        case 7: sweet = 0.0078125; break;
        case 6: sweet = 0.015625; break;
        case 5: sweet = 0.03125; break;
        case 4: sweet = 0.0625; break;
        case 3: sweet = 0.125; break;
        case 2: sweet = 0.25; break;
        case 1: sweet = 0.5; break;
        case 0: sweet = 1.0; break;
        case -1: sweet = 2.0; break;
        default: break;
    } //now we have our input trim

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;

        double sweetSample = inputSampleL;
        double sv = sweetSample; sweetSample = (sweetSample + savg[0]) * 0.5; savg[0] = sv;
        if (cycleEnd > 1)
        {
            sv = sweetSample; sweetSample = (sweetSample + savg[1]) * 0.5; savg[1] = sv;
            if (cycleEnd > 2)
            {
                sv = sweetSample; sweetSample = (sweetSample + savg[2]) * 0.5; savg[2] = sv;
                if (cycleEnd > 3)
                {
                    sv = sweetSample; sweetSample = (sweetSample + savg[3]) * 0.5; savg[3] = sv;
                }
            } //if undersampling code is present, this gives a simple averaging that stacks up
        } //when high sample rates are present, for a more intense aliasing reduction. PRE nonlinearity
        sweetSample = (sweetSample * sweetSample * sweet); //second harmonic (nonlinearity)
        sv = sweetSample; sweetSample = (sweetSample + savg[4]) * 0.5; savg[4] = sv;
        if (cycleEnd > 1)
        {
            sv = sweetSample; sweetSample = (sweetSample + savg[5]) * 0.5; savg[5] = sv;
            if (cycleEnd > 2)
            {
                sv = sweetSample; sweetSample = (sweetSample + savg[6]) * 0.5; savg[6] = sv;
                if (cycleEnd > 3)
                {
                    sv = sweetSample; sweetSample = (sweetSample + savg[7]) * 0.5; savg[7] = sv;
                }
            } //if undersampling code is present, this gives a simple averaging that stacks up
        } //when high sample rates are present, for a more intense aliasing reduction. POST nonlinearity
        inputSampleL -= sweetSample; //apply the filtered second harmonic correction

        sweetSample = inputSampleR;
        sv = sweetSample; sweetSample = (sweetSample + savg[8]) * 0.5; savg[8] = sv;
        if (cycleEnd > 1)
        {
            sv = sweetSample; sweetSample = (sweetSample + savg[9]) * 0.5; savg[9] = sv;
            if (cycleEnd > 2)
            {
                sv = sweetSample; sweetSample = (sweetSample + savg[10]) * 0.5; savg[10] = sv;
                if (cycleEnd > 3)
                {
                    sv = sweetSample; sweetSample = (sweetSample + savg[11]) * 0.5; savg[11] = sv;
                }
            } //if undersampling code is present, this gives a simple averaging that stacks up
        } //when high sample rates are present, for a more intense aliasing reduction. PRE nonlinearity
        sweetSample = (sweetSample * sweetSample * sweet); //second harmonic (nonlinearity)
        sv = sweetSample; sweetSample = (sweetSample + savg[12]) * 0.5; savg[12] = sv;
        if (cycleEnd > 1)
        {
            sv = sweetSample; sweetSample = (sweetSample + savg[13]) * 0.5; savg[13] = sv;
            if (cycleEnd > 2)
            {
                sv = sweetSample; sweetSample = (sweetSample + savg[14]) * 0.5; savg[14] = sv;
                if (cycleEnd > 3)
                {
                    sv = sweetSample; sweetSample = (sweetSample + savg[15]) * 0.5; savg[15] = sv;
                }
            } //if undersampling code is present, this gives a simple averaging that stacks up
        } //when high sample rates are present, for a more intense aliasing reduction. POST nonlinearity
        inputSampleR -= sweetSample; //apply the filtered second harmonic correction

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
