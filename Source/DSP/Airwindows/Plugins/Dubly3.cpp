/* ========================================
 *  Dubly3 - Dubly3.cpp
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Dubly3 —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter.
 * ======================================== */

#include "DSP/Airwindows/Plugins/Dubly3.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "input",  "Input",  0.5f },
        { "tilt",   "Tilt",   0.5f },
        { "shape",  "Shape",  0.5f },
        { "output", "Output", 0.5f },
    };
}

Dubly3::Dubly3() noexcept
    : AirwindowsPlugin ("dubly3", "Dubly3", kParameters)
{
}

void Dubly3::resetState() noexcept
{
    iirEncL = 0.0; iirEncR = 0.0;
    iirDecL = 0.0; iirDecR = 0.0;
    compEncL = 1.0; compEncR = 1.0;
    compDecL = 1.0; compDecR = 1.0;
    avgEncL = 0.0; avgEncR = 0.0;
    avgDecL = 0.0; avgDecR = 0.0;
}

void Dubly3::processStereo (const float* in1, const float* in2,
                            float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);
    const float B = param (kParamB);
    const float C = param (kParamC);
    const float D = param (kParamD);

    double overallscale = 1.0;
    overallscale /= 44100.0;
    overallscale *= getSampleRate();

    double inputGain = std::pow (A * 2.0, 2.0);
    double dublyAmount = B * 2.0;
    double outlyAmount = (1.0 - B) * -2.0;
    if (outlyAmount < -1.0) outlyAmount = -1.0;
    double iirEncFreq = (1.0 - C) / overallscale;
    double iirDecFreq = C / overallscale;
    double outputGain = D * 2.0;

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;

        if (inputGain != 1.0)
        {
            inputSampleL *= inputGain;
            inputSampleR *= inputGain;
        }

        //Dubly encode
        iirEncL = (iirEncL * (1.0 - iirEncFreq)) + (inputSampleL * iirEncFreq);
        double highPart = ((inputSampleL - iirEncL) * 2.848);
        highPart += avgEncL; avgEncL = (inputSampleL - iirEncL) * 1.152;
        if (highPart > 1.0) highPart = 1.0; if (highPart < -1.0) highPart = -1.0;
        double dubly = std::fabs (highPart);
        if (dubly > 0.0)
        {
            double adjust = std::log (1.0 + (255.0 * dubly)) / 2.40823996531;
            if (adjust > 0.0) dubly /= adjust;
            compEncL = (compEncL * (1.0 - iirEncFreq)) + (dubly * iirEncFreq);
            inputSampleL += ((highPart * compEncL) * dublyAmount);
        } //end Dubly encode L
        iirEncR = (iirEncR * (1.0 - iirEncFreq)) + (inputSampleR * iirEncFreq);
        highPart = ((inputSampleR - iirEncR) * 2.848);
        highPart += avgEncR; avgEncR = (inputSampleR - iirEncR) * 1.152;
        if (highPart > 1.0) highPart = 1.0; if (highPart < -1.0) highPart = -1.0;
        dubly = std::fabs (highPart);
        if (dubly > 0.0)
        {
            double adjust = std::log (1.0 + (255.0 * dubly)) / 2.40823996531;
            if (adjust > 0.0) dubly /= adjust;
            compEncR = (compEncR * (1.0 - iirEncFreq)) + (dubly * iirEncFreq);
            inputSampleR += ((highPart * compEncR) * dublyAmount);
        } //end Dubly encode R

        if (inputSampleL > 1.57079633) inputSampleL = 1.57079633;
        if (inputSampleL < -1.57079633) inputSampleL = -1.57079633;
        inputSampleL = std::sin (inputSampleL);
        if (inputSampleR > 1.57079633) inputSampleR = 1.57079633;
        if (inputSampleR < -1.57079633) inputSampleR = -1.57079633;
        inputSampleR = std::sin (inputSampleR);

        //Dubly decode
        iirDecL = (iirDecL * (1.0 - iirDecFreq)) + (inputSampleL * iirDecFreq);
        highPart = ((inputSampleL - iirDecL) * 2.628);
        highPart += avgDecL; avgDecL = (inputSampleL - iirDecL) * 1.372;
        if (highPart > 1.0) highPart = 1.0; if (highPart < -1.0) highPart = -1.0;
        dubly = std::fabs (highPart);
        if (dubly > 0.0)
        {
            double adjust = std::log (1.0 + (255.0 * dubly)) / 2.40823996531;
            if (adjust > 0.0) dubly /= adjust;
            compDecL = (compDecL * (1.0 - iirDecFreq)) + (dubly * iirDecFreq);
            inputSampleL += ((highPart * compDecL) * outlyAmount);
        } //end Dubly decode L
        iirDecR = (iirDecR * (1.0 - iirDecFreq)) + (inputSampleR * iirDecFreq);
        highPart = ((inputSampleR - iirDecR) * 2.628);
        highPart += avgDecR; avgDecR = (inputSampleR - iirDecR) * 1.372;
        if (highPart > 1.0) highPart = 1.0; if (highPart < -1.0) highPart = -1.0;
        dubly = std::fabs (highPart);
        if (dubly > 0.0)
        {
            double adjust = std::log (1.0 + (255.0 * dubly)) / 2.40823996531;
            if (adjust > 0.0) dubly /= adjust;
            compDecR = (compDecR * (1.0 - iirDecFreq)) + (dubly * iirDecFreq);
            inputSampleR += ((highPart * compDecR) * outlyAmount);
        } //end Dubly decode R

        if (outputGain != 1.0)
        {
            inputSampleL *= outputGain;
            inputSampleR *= outputGain;
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
