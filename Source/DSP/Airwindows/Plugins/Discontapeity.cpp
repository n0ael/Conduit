/* ========================================
 *  Discontapeity - Discontapeity.cpp
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Discontapeity —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter.
 * ======================================== */

#include "DSP/Airwindows/Plugins/Discontapeity.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "more", "More", 0.0f },
    };
}

Discontapeity::Discontapeity() noexcept
    : AirwindowsPlugin ("discontapeity", "Discontapeity", kParameters)
{
}

void Discontapeity::resetState() noexcept
{
    for (double& v : dBaL) v = 0.0;
    for (double& v : dBaR) v = 0.0;
    dBaPosL = 0.0;
    dBaPosR = 0.0;
    dBaXL = 1;
    dBaXR = 1;
}

void Discontapeity::processStereo (const float* in1, const float* in2,
                                   float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);

    double overallscale = 1.0;
    overallscale /= 44100.0;
    overallscale *= getSampleRate();

    double moreDiscontinuity = std::fmax (std::pow (A * 0.42, 3.0) * overallscale, 0.00001);
    double moreTapeHack = (A * 1.4152481) + 1.2;

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;

        //begin Discontinuity section
        inputSampleL *= moreDiscontinuity;
        dBaL[dBaXL] = inputSampleL; dBaPosL *= 0.5; dBaPosL += std::fabs ((inputSampleL * ((inputSampleL * 0.25) - 0.5)) * 0.5);
        dBaPosL = std::fmin (dBaPosL, 1.0);
        int dBdly = (int) std::floor (dBaPosL * dscBuf);
        double dBi = (dBaPosL * dscBuf) - dBdly;
        inputSampleL = dBaL[dBaXL - dBdly + ((dBaXL - dBdly < 0) ? dscBuf : 0)] * (1.0 - dBi);
        dBdly++; inputSampleL += dBaL[dBaXL - dBdly + ((dBaXL - dBdly < 0) ? dscBuf : 0)] * dBi;
        dBaXL++; if (dBaXL < 0 || dBaXL >= dscBuf) dBaXL = 0;
        inputSampleL /= moreDiscontinuity;
        //end Discontinuity section, begin TapeHack section
        inputSampleL = std::fmax (std::fmin (inputSampleL * moreTapeHack, 2.305929007734908), -2.305929007734908);
        double addtwo = inputSampleL * inputSampleL;
        double empower = inputSampleL * addtwo; // inputSampleL to the third power
        inputSampleL -= (empower / 6.0);
        empower *= addtwo; // to the fifth power
        inputSampleL += (empower / 69.0);
        empower *= addtwo; //seventh
        inputSampleL -= (empower / 2530.08);
        empower *= addtwo; //ninth
        inputSampleL += (empower / 224985.6);
        empower *= addtwo; //eleventh
        inputSampleL -= (empower / 9979200.0);
        //this is a degenerate form of a Taylor Series to approximate sin()
        inputSampleL *= 0.9239;
        //end TapeHack section

        //begin Discontinuity section
        inputSampleR *= moreDiscontinuity;
        dBaR[dBaXR] = inputSampleR; dBaPosR *= 0.5; dBaPosR += std::fabs ((inputSampleR * ((inputSampleR * 0.25) - 0.5)) * 0.5);
        dBaPosR = std::fmin (dBaPosR, 1.0);
        dBdly = (int) std::floor (dBaPosR * dscBuf);
        dBi = (dBaPosR * dscBuf) - dBdly;
        inputSampleR = dBaR[dBaXR - dBdly + ((dBaXR - dBdly < 0) ? dscBuf : 0)] * (1.0 - dBi);
        dBdly++; inputSampleR += dBaR[dBaXR - dBdly + ((dBaXR - dBdly < 0) ? dscBuf : 0)] * dBi;
        dBaXR++; if (dBaXR < 0 || dBaXR >= dscBuf) dBaXR = 0;
        inputSampleR /= moreDiscontinuity;
        //end Discontinuity section, begin TapeHack section
        inputSampleR = std::fmax (std::fmin (inputSampleR * moreTapeHack, 2.305929007734908), -2.305929007734908);
        addtwo = inputSampleR * inputSampleR;
        empower = inputSampleR * addtwo; // inputSampleR to the third power
        inputSampleR -= (empower / 6.0);
        empower *= addtwo; // to the fifth power
        inputSampleR += (empower / 69.0);
        empower *= addtwo; //seventh
        inputSampleR -= (empower / 2530.08);
        empower *= addtwo; //ninth
        inputSampleR += (empower / 224985.6);
        empower *= addtwo; //eleventh
        inputSampleR -= (empower / 9979200.0);
        //this is a degenerate form of a Taylor Series to approximate sin()
        inputSampleR *= 0.9239;
        //end TapeHack section

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
