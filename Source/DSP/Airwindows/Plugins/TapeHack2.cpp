/* ========================================
 *  TapeHack2 - TapeHack2.cpp
 *  Copyright (c) Airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/TapeHack2 —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter
 *  (entspricht dem processDoubleReplacing-Pfad des Originals).
 * ======================================== */

#include "DSP/Airwindows/Plugins/TapeHack2.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "input",   "Input",   0.1f },
        { "output",  "Output",  1.0f },
        { "dry_wet", "Dry/Wet", 1.0f },
    };
}

TapeHack2::TapeHack2() noexcept
    : AirwindowsPlugin ("tapehack2", "TapeHack2", kParameters)
{
}

void TapeHack2::resetState() noexcept
{
    avg32L.fill (0.0); avg32R.fill (0.0);
    avg16L.fill (0.0); avg16R.fill (0.0);
    avg8L.fill (0.0);  avg8R.fill (0.0);
    avg4L.fill (0.0);  avg4R.fill (0.0);
    avg2L.fill (0.0);  avg2R.fill (0.0);
    post32L.fill (0.0); post32R.fill (0.0);
    post16L.fill (0.0); post16R.fill (0.0);
    post8L.fill (0.0);  post8R.fill (0.0);
    post4L.fill (0.0);  post4R.fill (0.0);
    post2L.fill (0.0);  post2R.fill (0.0);
    lastDarkL = 0.0;
    lastDarkR = 0.0;
    avgPos = 0;
}

void TapeHack2::processStereo (const float* in1, const float* in2,
                               float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);
    const float B = param (kParamB);
    const float C = param (kParamC);

    double overallscale = 1.0;
    overallscale /= 44100.0;
    overallscale *= getSampleRate();
    int spacing = (int) std::floor (overallscale * 2.0);
    if (spacing < 2) spacing = 2;
    if (spacing > 32) spacing = 32;

    double inputGain = A * 10.0;
    double outputGain = B * 0.9239;
    double wet = C;

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;
        double drySampleL = inputSampleL;
        double drySampleR = inputSampleR;

        inputSampleL *= inputGain;
        inputSampleR *= inputGain;
        double darkSampleL = inputSampleL;
        double darkSampleR = inputSampleR;
        if (avgPos > 31) avgPos = 0;
        if (spacing > 31)
        {
            avg32L[(size_t) avgPos] = darkSampleL; avg32R[(size_t) avgPos] = darkSampleR;
            darkSampleL = 0.0; darkSampleR = 0.0;
            for (int x = 0; x < 32; ++x) { darkSampleL += avg32L[(size_t) x]; darkSampleR += avg32R[(size_t) x]; }
            darkSampleL /= 32.0; darkSampleR /= 32.0;
        }
        if (spacing > 15)
        {
            avg16L[(size_t) (avgPos % 16)] = darkSampleL; avg16R[(size_t) (avgPos % 16)] = darkSampleR;
            darkSampleL = 0.0; darkSampleR = 0.0;
            for (int x = 0; x < 16; ++x) { darkSampleL += avg16L[(size_t) x]; darkSampleR += avg16R[(size_t) x]; }
            darkSampleL /= 16.0; darkSampleR /= 16.0;
        }
        if (spacing > 7)
        {
            avg8L[(size_t) (avgPos % 8)] = darkSampleL; avg8R[(size_t) (avgPos % 8)] = darkSampleR;
            darkSampleL = 0.0; darkSampleR = 0.0;
            for (int x = 0; x < 8; ++x) { darkSampleL += avg8L[(size_t) x]; darkSampleR += avg8R[(size_t) x]; }
            darkSampleL /= 8.0; darkSampleR /= 8.0;
        }
        if (spacing > 3)
        {
            avg4L[(size_t) (avgPos % 4)] = darkSampleL; avg4R[(size_t) (avgPos % 4)] = darkSampleR;
            darkSampleL = 0.0; darkSampleR = 0.0;
            for (int x = 0; x < 4; ++x) { darkSampleL += avg4L[(size_t) x]; darkSampleR += avg4R[(size_t) x]; }
            darkSampleL /= 4.0; darkSampleR /= 4.0;
        }
        if (spacing > 1)
        {
            avg2L[(size_t) (avgPos % 2)] = darkSampleL; avg2R[(size_t) (avgPos % 2)] = darkSampleR;
            darkSampleL = 0.0; darkSampleR = 0.0;
            for (int x = 0; x < 2; ++x) { darkSampleL += avg2L[(size_t) x]; darkSampleR += avg2R[(size_t) x]; }
            darkSampleL /= 2.0; darkSampleR /= 2.0;
        } // only update avgPos after the post-distortion filter stage

        double avgSlewL = std::fmin (std::fabs (lastDarkL - inputSampleL) * 0.12 * overallscale, 1.0);
        avgSlewL = 1.0 - (1.0 - avgSlewL * 1.0 - avgSlewL);
        inputSampleL = (inputSampleL * (1.0 - avgSlewL)) + (darkSampleL * avgSlewL);
        lastDarkL = darkSampleL;
        double avgSlewR = std::fmin (std::fabs (lastDarkR - inputSampleR) * 0.12 * overallscale, 1.0);
        avgSlewR = 1.0 - (1.0 - avgSlewR * 1.0 - avgSlewR);
        inputSampleR = (inputSampleR * (1.0 - avgSlewR)) + (darkSampleR * avgSlewR);
        lastDarkR = darkSampleR;

        inputSampleL = std::fmax (std::fmin (inputSampleL, 2.305929007734908), -2.305929007734908);
        double addtwo = inputSampleL * inputSampleL;
        double empower = inputSampleL * addtwo; // inputSampleL to the third power
        inputSampleL -= (empower / 6.0);
        empower *= addtwo; // to the fifth power
        inputSampleL += (empower / 69.0);
        empower *= addtwo; // seventh
        inputSampleL -= (empower / 2530.08);
        empower *= addtwo; // ninth
        inputSampleL += (empower / 224985.6);
        empower *= addtwo; // eleventh
        inputSampleL -= (empower / 9979200.0);
        // this is a degenerate form of a Taylor Series to approximate sin()
        inputSampleR = std::fmax (std::fmin (inputSampleR, 2.305929007734908), -2.305929007734908);
        addtwo = inputSampleR * inputSampleR;
        empower = inputSampleR * addtwo; // inputSampleR to the third power
        inputSampleR -= (empower / 6.0);
        empower *= addtwo; // to the fifth power
        inputSampleR += (empower / 69.0);
        empower *= addtwo; // seventh
        inputSampleR -= (empower / 2530.08);
        empower *= addtwo; // ninth
        inputSampleR += (empower / 224985.6);
        empower *= addtwo; // eleventh
        inputSampleR -= (empower / 9979200.0);
        // this is a degenerate form of a Taylor Series to approximate sin()

        darkSampleL = inputSampleL;
        darkSampleR = inputSampleR;
        if (avgPos > 31) avgPos = 0;
        if (spacing > 31)
        {
            post32L[(size_t) avgPos] = darkSampleL; post32R[(size_t) avgPos] = darkSampleR;
            darkSampleL = 0.0; darkSampleR = 0.0;
            for (int x = 0; x < 32; ++x) { darkSampleL += post32L[(size_t) x]; darkSampleR += post32R[(size_t) x]; }
            darkSampleL /= 32.0; darkSampleR /= 32.0;
        }
        if (spacing > 15)
        {
            post16L[(size_t) (avgPos % 16)] = darkSampleL; post16R[(size_t) (avgPos % 16)] = darkSampleR;
            darkSampleL = 0.0; darkSampleR = 0.0;
            for (int x = 0; x < 16; ++x) { darkSampleL += post16L[(size_t) x]; darkSampleR += post16R[(size_t) x]; }
            darkSampleL /= 16.0; darkSampleR /= 16.0;
        }
        if (spacing > 7)
        {
            post8L[(size_t) (avgPos % 8)] = darkSampleL; post8R[(size_t) (avgPos % 8)] = darkSampleR;
            darkSampleL = 0.0; darkSampleR = 0.0;
            for (int x = 0; x < 8; ++x) { darkSampleL += post8L[(size_t) x]; darkSampleR += post8R[(size_t) x]; }
            darkSampleL /= 8.0; darkSampleR /= 8.0;
        }
        if (spacing > 3)
        {
            post4L[(size_t) (avgPos % 4)] = darkSampleL; post4R[(size_t) (avgPos % 4)] = darkSampleR;
            darkSampleL = 0.0; darkSampleR = 0.0;
            for (int x = 0; x < 4; ++x) { darkSampleL += post4L[(size_t) x]; darkSampleR += post4R[(size_t) x]; }
            darkSampleL /= 4.0; darkSampleR /= 4.0;
        }
        if (spacing > 1)
        {
            post2L[(size_t) (avgPos % 2)] = darkSampleL; post2R[(size_t) (avgPos % 2)] = darkSampleR;
            darkSampleL = 0.0; darkSampleR = 0.0;
            for (int x = 0; x < 2; ++x) { darkSampleL += post2L[(size_t) x]; darkSampleR += post2R[(size_t) x]; }
            darkSampleL /= 2.0; darkSampleR /= 2.0;
        }
        ++avgPos;
        inputSampleL = (inputSampleL * (1.0 - avgSlewL)) + (darkSampleL * avgSlewL);
        inputSampleR = (inputSampleR * (1.0 - avgSlewR)) + (darkSampleR * avgSlewR);
        // use the previously calculated depth of the filter

        inputSampleL = (inputSampleL * outputGain * wet) + (drySampleL * (1.0 - wet));
        inputSampleR = (inputSampleR * outputGain * wet) + (drySampleR * (1.0 - wet));

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
