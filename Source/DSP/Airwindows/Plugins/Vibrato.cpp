/* ========================================
 *  Vibrato - Vibrato.cpp
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Vibrato —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter
 *  (entspricht dem processDoubleReplacing-Pfad des Originals).
 * ======================================== */

#include "DSP/Airwindows/Plugins/Vibrato.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "speed",   "Speed",   0.3f },
        { "depth",   "Depth",   0.0f },
        { "fmspeed", "FMSpeed", 0.4f },
        { "fmdepth", "FMDepth", 0.0f },
        { "inv_wet", "Inv/Wet", 1.0f },
    };
}

Vibrato::Vibrato() noexcept
    : AirwindowsPlugin ("vibrato", "Vibrato", kParameters)
{
}

void Vibrato::resetState() noexcept
{
    pL.fill (0.0);
    pR.fill (0.0);
    sweep = 3.141592653589793238 / 2.0;
    sweepB = 3.141592653589793238 / 2.0;
    gcount = 0;

    airPrevL = 0.0;
    airEvenL = 0.0;
    airOddL = 0.0;
    airFactorL = 0.0;
    airPrevR = 0.0;
    airEvenR = 0.0;
    airOddR = 0.0;
    airFactorR = 0.0;

    flip = false;
}

void Vibrato::processStereo (const float* in1, const float* in2,
                             float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);
    const float B = param (kParamB);
    const float C = param (kParamC);
    const float D = param (kParamD);
    const float E = param (kParamE);

    double speed = std::pow (0.1 + A, 6);
    double depth = (std::pow (B, 3) / std::sqrt (speed)) * 4.0;
    double speedB = std::pow (0.1 + C, 6);
    double depthB = std::pow (D, 3) / std::sqrt (speedB);
    double tupi = 3.141592653589793238 * 2.0;
    double wet = (E * 2.0) - 1.0; // note: inv/dry/wet

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;
        double drySampleL = inputSampleL;
        double drySampleR = inputSampleR;

        airFactorL = airPrevL - inputSampleL;
        airFactorR = airPrevR - inputSampleR;

        if (flip)
        {
            airEvenL += airFactorL; airOddL -= airFactorL; airFactorL = airEvenL;
            airEvenR += airFactorR; airOddR -= airFactorR; airFactorR = airEvenR;
        }
        else
        {
            airOddL += airFactorL; airEvenL -= airFactorL; airFactorL = airOddL;
            airOddR += airFactorR; airEvenR -= airFactorR; airFactorR = airOddR;
        }

        airOddL = (airOddL - ((airOddL - airEvenL) / 256.0)) / 1.0001;
        airOddR = (airOddR - ((airOddR - airEvenR) / 256.0)) / 1.0001;
        airEvenL = (airEvenL - ((airEvenL - airOddL) / 256.0)) / 1.0001;
        airEvenR = (airEvenR - ((airEvenR - airOddR) / 256.0)) / 1.0001;
        airPrevL = inputSampleL;
        airPrevR = inputSampleR;
        inputSampleL += airFactorL;
        inputSampleR += airFactorR;

        flip = ! flip;
        // air, compensates for loss of highs in the interpolation

        if (gcount < 1 || gcount > 8192) { gcount = 8192; }
        int count = gcount;
        pL[(size_t) (count + 8192)] = pL[(size_t) count] = inputSampleL;
        pR[(size_t) (count + 8192)] = pR[(size_t) count] = inputSampleR;

        double offset = depth + (depth * std::sin (sweep));
        count += (int) std::floor (offset);

        inputSampleL = pL[(size_t) count] * (1.0 - (offset - std::floor (offset))); // less as value moves away from .0
        inputSampleL += pL[(size_t) (count + 1)]; // we can assume always using this in one way or another?
        inputSampleL += pL[(size_t) (count + 2)] * (offset - std::floor (offset)); // greater as value moves away from .0
        inputSampleL -= ((pL[(size_t) count] - pL[(size_t) (count + 1)]) - (pL[(size_t) (count + 1)] - pL[(size_t) (count + 2)])) / 50.0; // interpolation hacks 'r us
        inputSampleL *= 0.5; // gain trim

        inputSampleR = pR[(size_t) count] * (1.0 - (offset - std::floor (offset))); // less as value moves away from .0
        inputSampleR += pR[(size_t) (count + 1)]; // we can assume always using this in one way or another?
        inputSampleR += pR[(size_t) (count + 2)] * (offset - std::floor (offset)); // greater as value moves away from .0
        inputSampleR -= ((pR[(size_t) count] - pR[(size_t) (count + 1)]) - (pR[(size_t) (count + 1)] - pR[(size_t) (count + 2)])) / 50.0; // interpolation hacks 'r us
        inputSampleR *= 0.5; // gain trim

        sweep += (speed + (speedB * std::sin (sweepB) * depthB));
        sweepB += speedB;
        if (sweep > tupi) { sweep -= tupi; }
        if (sweep < 0.0) { sweep += tupi; } // through zero FM
        if (sweepB > tupi) { sweepB -= tupi; }
        --gcount;
        // still scrolling through the samples, remember

        if (wet != 1.0)
        {
            inputSampleL = (inputSampleL * wet) + (drySampleL * (1.0 - std::fabs (wet)));
            inputSampleR = (inputSampleR * wet) + (drySampleR * (1.0 - std::fabs (wet)));
        }
        // Inv/Dry/Wet control

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
