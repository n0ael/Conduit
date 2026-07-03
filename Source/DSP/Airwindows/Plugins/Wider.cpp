/* ========================================
 *  Wider - Wider.cpp
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Wider —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter
 *  (entspricht dem processDoubleReplacing-Pfad des Originals).
 * ======================================== */

#include "DSP/Airwindows/Plugins/Wider.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "width",   "Width",   0.5f },
        { "center",  "Center",  0.5f },
        { "dry_wet", "Dry/Wet", 1.0f },
    };
}

Wider::Wider() noexcept
    : AirwindowsPlugin ("wider", "Wider", kParameters)
{
}

void Wider::resetState() noexcept
{
    p.fill (0.0);
    count = 0;
}

void Wider::processStereo (const float* in1, const float* in2,
                           float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);
    const float B = param (kParamB);
    const float C = param (kParamC);

    double overallscale = 1.0;
    overallscale /= 44100.0;
    overallscale *= getSampleRate();

    double densityside = (A * 2.0) - 1.0;
    double densitymid = (B * 2.0) - 1.0;
    double wet = C;
    // removed extra dry variable
    wet *= 0.5; // we make mid-side by adding/subtracting both channels into each channel
    // and that's why we gotta divide it by 2: otherwise everything's doubled. So, premultiply it to save an extra 'math'
    double offset = (densityside - densitymid) / 2;
    if (offset > 0) offset = std::sin (offset);
    if (offset < 0) offset = -std::sin (-offset);
    offset = -(std::pow (offset, 4) * 20 * overallscale);
    int near = (int) std::floor (std::fabs (offset));
    double farLevel = std::fabs (offset) - near;
    int far = near + 1;
    double nearLevel = 1.0 - farLevel;
    double bridgerectifier;
    // interpolating the sample

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;
        double drySampleL = inputSampleL;
        double drySampleR = inputSampleR;
        // assign working variables
        double mid = inputSampleL + inputSampleR;
        double side = inputSampleL - inputSampleR;
        // assign mid and side. Now, High Impact code

        if (densityside != 0.0)
        {
            double out = std::fabs (densityside);
            bridgerectifier = std::fabs (side) * 1.57079633;
            if (bridgerectifier > 1.57079633) bridgerectifier = 1.57079633;
            // max value for sine function
            if (densityside > 0) bridgerectifier = std::sin (bridgerectifier);
            else bridgerectifier = 1 - std::cos (bridgerectifier);
            // produce either boosted or starved version
            if (side > 0) side = (side * (1 - out)) + (bridgerectifier * out);
            else side = (side * (1 - out)) - (bridgerectifier * out);
            // blend according to density control
        }

        if (densitymid != 0.0)
        {
            double out = std::fabs (densitymid);
            bridgerectifier = std::fabs (mid) * 1.57079633;
            if (bridgerectifier > 1.57079633) bridgerectifier = 1.57079633;
            // max value for sine function
            if (densitymid > 0) bridgerectifier = std::sin (bridgerectifier);
            else bridgerectifier = 1 - std::cos (bridgerectifier);
            // produce either boosted or starved version
            if (mid > 0) mid = (mid * (1 - out)) + (bridgerectifier * out);
            else mid = (mid * (1 - out)) - (bridgerectifier * out);
            // blend according to density control
        }

        if (count < 1 || count > 2048) { count = 2048; }
        if (offset > 0)
        {
            p[(size_t) (count + 2048)] = p[(size_t) count] = mid;
            mid = p[(size_t) (count + near)] * nearLevel;
            mid += p[(size_t) (count + far)] * farLevel;
        }

        if (offset < 0)
        {
            p[(size_t) (count + 2048)] = p[(size_t) count] = side;
            side = p[(size_t) (count + near)] * nearLevel;
            side += p[(size_t) (count + far)] * farLevel;
        }
        count -= 1;

        inputSampleL = (drySampleL * (1.0 - wet)) + ((mid + side) * wet);
        inputSampleR = (drySampleR * (1.0 - wet)) + ((mid - side) * wet);

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
