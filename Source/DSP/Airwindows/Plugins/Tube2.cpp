/* ========================================
 *  Tube2 - Tube2.cpp
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Tube2 —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter
 *  (entspricht dem processDoubleReplacing-Pfad des Originals).
 * ======================================== */

#include "DSP/Airwindows/Plugins/Tube2.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "input", "Input", 0.5f },
        { "tube",  "Tube",  0.5f },
    };
}

Tube2::Tube2() noexcept
    : AirwindowsPlugin ("tube2", "Tube2", kParameters)
{
}

void Tube2::resetState() noexcept
{
    previousSampleA = 0.0;
    previousSampleB = 0.0;
    previousSampleC = 0.0;
    previousSampleD = 0.0;
    previousSampleE = 0.0;
    previousSampleF = 0.0;
}

void Tube2::processStereo (const float* in1, const float* in2,
                           float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);
    const float B = param (kParamB);

    double overallscale = 1.0;
    overallscale /= 44100.0;
    overallscale *= getSampleRate();

    double inputPad = A;
    double iterations = 1.0 - B;
    int powerfactor = (int) ((9.0 * iterations) + 1);
    double asymPad = (double) powerfactor;
    double gainscaling = 1.0 / (double) (powerfactor + 1);
    double outputscaling = 1.0 + (1.0 / (double) (powerfactor));

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;

        if (inputPad < 1.0)
        {
            inputSampleL *= inputPad;
            inputSampleR *= inputPad;
        }

        if (overallscale > 1.9)
        {
            double stored = inputSampleL;
            inputSampleL += previousSampleA; previousSampleA = stored; inputSampleL *= 0.5;
            stored = inputSampleR;
            inputSampleR += previousSampleB; previousSampleB = stored; inputSampleR *= 0.5;
        } // for high sample rates on this plugin we are going to do a simple average

        if (inputSampleL > 1.0) inputSampleL = 1.0;
        if (inputSampleL < -1.0) inputSampleL = -1.0;
        if (inputSampleR > 1.0) inputSampleR = 1.0;
        if (inputSampleR < -1.0) inputSampleR = -1.0;

        // flatten bottom, point top of sine waveshaper L
        inputSampleL /= asymPad;
        double sharpen = -inputSampleL;
        if (sharpen > 0.0) sharpen = 1.0 + std::sqrt (sharpen);
        else sharpen = 1.0 - std::sqrt (-sharpen);
        inputSampleL -= inputSampleL * std::fabs (inputSampleL) * sharpen * 0.25;
        // this will take input from exactly -1.0 to 1.0 max
        inputSampleL *= asymPad;
        // flatten bottom, point top of sine waveshaper R
        inputSampleR /= asymPad;
        sharpen = -inputSampleR;
        if (sharpen > 0.0) sharpen = 1.0 + std::sqrt (sharpen);
        else sharpen = 1.0 - std::sqrt (-sharpen);
        inputSampleR -= inputSampleR * std::fabs (inputSampleR) * sharpen * 0.25;
        // this will take input from exactly -1.0 to 1.0 max
        inputSampleR *= asymPad;
        // end first asym section: later boosting can mitigate the extreme
        // softclipping of one side of the wave
        // and we are asym clipping more when Tube is cranked, to compensate

        // original Tube algorithm: powerfactor widens the more linear region of the wave
        double factor = inputSampleL; // Left channel
        for (int x = 0; x < powerfactor; ++x) factor *= inputSampleL;
        if ((powerfactor % 2 == 1) && (inputSampleL != 0.0)) factor = (factor / inputSampleL) * std::fabs (inputSampleL);
        factor *= gainscaling;
        inputSampleL -= factor;
        inputSampleL *= outputscaling;
        factor = inputSampleR; // Right channel
        for (int x = 0; x < powerfactor; ++x) factor *= inputSampleR;
        if ((powerfactor % 2 == 1) && (inputSampleR != 0.0)) factor = (factor / inputSampleR) * std::fabs (inputSampleR);
        factor *= gainscaling;
        inputSampleR -= factor;
        inputSampleR *= outputscaling;

        if (overallscale > 1.9)
        {
            double stored = inputSampleL;
            inputSampleL += previousSampleC; previousSampleC = stored; inputSampleL *= 0.5;
            stored = inputSampleR;
            inputSampleR += previousSampleD; previousSampleD = stored; inputSampleR *= 0.5;
        } // for high sample rates on this plugin we are going to do a simple average
        // end original Tube. Now we have a boosted fat sound peaking at 0dB exactly

        // hysteresis and spiky fuzz L
        double slew = previousSampleE - inputSampleL;
        if (overallscale > 1.9)
        {
            double stored = inputSampleL;
            inputSampleL += previousSampleE; previousSampleE = stored; inputSampleL *= 0.5;
        }
        else previousSampleE = inputSampleL; // for this, need previousSampleC always
        if (slew > 0.0) slew = 1.0 + (std::sqrt (slew) * 0.5);
        else slew = 1.0 - (std::sqrt (-slew) * 0.5);
        inputSampleL -= inputSampleL * std::fabs (inputSampleL) * slew * gainscaling;
        // reusing gainscaling that's part of another algorithm
        if (inputSampleL > 0.52) inputSampleL = 0.52;
        if (inputSampleL < -0.52) inputSampleL = -0.52;
        inputSampleL *= 1.923076923076923;
        // hysteresis and spiky fuzz R
        slew = previousSampleF - inputSampleR;
        if (overallscale > 1.9)
        {
            double stored = inputSampleR;
            inputSampleR += previousSampleF; previousSampleF = stored; inputSampleR *= 0.5;
        }
        else previousSampleF = inputSampleR; // for this, need previousSampleC always
        if (slew > 0.0) slew = 1.0 + (std::sqrt (slew) * 0.5);
        else slew = 1.0 - (std::sqrt (-slew) * 0.5);
        inputSampleR -= inputSampleR * std::fabs (inputSampleR) * slew * gainscaling;
        // reusing gainscaling that's part of another algorithm
        if (inputSampleR > 0.52) inputSampleR = 0.52;
        if (inputSampleR < -0.52) inputSampleR = -0.52;
        inputSampleR *= 1.923076923076923;
        // end hysteresis and spiky fuzz section

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
