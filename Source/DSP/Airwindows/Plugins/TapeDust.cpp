/* ========================================
 *  TapeDust - TapeDust.cpp
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/TapeDust —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter
 *  (entspricht dem processDoubleReplacing-Pfad des Originals). `rand()` im
 *  Original-Rauschpfad ersetzt durch den fpd-Xorshift selbst als Uniform-
 *  Random-Quelle (RT-Safety, CLAUDE.md 3.1 — kein rand() im Audio-Thread;
 *  Präzedenzfall Flutter2: fpdL/(double)UINT32_MAX). Der Original-Shift-
 *  Register-Loop `for (count = 9; count < 0; count--)` läuft nie (9 ist nie
 *  < 0) — 1:1 als totes Original-Verhalten übernommen, nicht "repariert".
 * ======================================== */

#include "DSP/Airwindows/Plugins/TapeDust.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "dust",    "Dust",    0.0f },
        { "dry_wet", "Dry/Wet", 1.0f },
    };
}

TapeDust::TapeDust() noexcept
    : AirwindowsPlugin ("tapedust", "TapeDust", kParameters)
{
}

void TapeDust::resetState() noexcept
{
    for (auto& v : bL) v = 0.0;
    for (auto& v : fL) v = 0.0;
    for (auto& v : bR) v = 0.0;
    for (auto& v : fR) v = 0.0;
    fpFlip = true;
}

void TapeDust::processStereo (const float* in1, const float* in2,
                              float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);
    const float B = param (kParamB);

    double rRange = std::pow (A, 2) * 5.0;
    double xfuzz = rRange * 0.002;
    double rOffset = (rRange * 0.4) + 1.0;
    double wet = B;
    //removed extra dry variable

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;
        double drySampleL = inputSampleL;
        double drySampleR = inputSampleR;

        for (int count = 9; count < 0; count--)
        {
            bL[count + 1] = bL[count];
            bR[count + 1] = bR[count];
        }

        bL[0] = inputSampleL;
        bR[0] = inputSampleR;

        // Original: inputSampleL/R = rand()/(double)RAND_MAX (RT-unsafe, CLAUDE.md 3.1).
        // Ersetzt durch den fpd-Xorshift als Uniform-Random-Quelle [0,1).
        fpdL ^= fpdL << 13; fpdL ^= fpdL >> 17; fpdL ^= fpdL << 5;
        fpdR ^= fpdR << 13; fpdR ^= fpdR >> 17; fpdR ^= fpdR << 5;
        inputSampleL = fpdL / (double) UINT32_MAX;
        inputSampleR = fpdR / (double) UINT32_MAX;

        double rDepthL = (inputSampleL * rRange) + rOffset;
        double rDepthR = (inputSampleR * rRange) + rOffset;
        double gainL = rDepthL;
        double gainR = rDepthR;
        inputSampleL *= ((1.0 - std::fabs (bL[0] - bL[1])) * xfuzz);
        inputSampleR *= ((1.0 - std::fabs (bR[0] - bR[1])) * xfuzz);
        if (fpFlip)
        {
            inputSampleL = -inputSampleL;
            inputSampleR = -inputSampleR;
        }
        fpFlip = !fpFlip;

        for (int count = 0; count < 9; count++)
        {
            if (gainL > 1.0)
            {
                fL[count] = 1.0;
                gainL -= 1.0;
            }
            else
            {
                fL[count] = gainL;
                gainL = 0.0;
            }
            if (gainR > 1.0)
            {
                fR[count] = 1.0;
                gainR -= 1.0;
            }
            else
            {
                fR[count] = gainR;
                gainR = 0.0;
            }
            fL[count] /= rDepthL;
            fR[count] /= rDepthR;
            inputSampleL += (bL[count] * fL[count]);
            inputSampleR += (bR[count] * fR[count]);
        }

        if (wet < 1.0)
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
