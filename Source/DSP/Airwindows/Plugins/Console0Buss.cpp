/* ========================================
 *  Console0Buss - Console0Buss.cpp
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Console0Buss —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter
 *  (entspricht dem processDoubleReplacing-Pfad des Originals).
 * ======================================== */

#include "DSP/Airwindows/Plugins/Console0Buss.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "vol", "Vol", 0.5f },
        { "pan", "Pan", 0.5f },
    };
}

Console0Buss::Console0Buss() noexcept
    : AirwindowsPlugin ("console0buss", "Console0Buss", kParameters)
{
}

void Console0Buss::resetState() noexcept
{
    avgAL = avgAR = avgBL = avgBR = 0.0;
}

void Console0Buss::processStereo (const float* in1, const float* in2,
                                  float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);
    const float B = param (kParamB);

    double gainControl = (A * 0.5) + 0.05; //0.0 to 1.0
    int gainBits = 20; //start beyond maximum attenuation
    if (gainControl > 0.0) gainBits = (int) std::floor (1.0 / gainControl);
    int bitshiftL = gainBits - 3;
    int bitshiftR = gainBits - 3;
    double panControl = (B * 2.0) - 1.0; //-1.0 to 1.0
    double panAttenuation = (1.0 - std::fabs (panControl));
    int panBits = 20; //start centered
    if (panAttenuation > 0.0) panBits = (int) std::floor (1.0 / panAttenuation);
    if (panControl > 0.25) bitshiftL += panBits;
    if (panControl < -0.25) bitshiftR += panBits;
    if (bitshiftL < -2) bitshiftL = -2; if (bitshiftL > 17) bitshiftL = 17;
    if (bitshiftR < -2) bitshiftR = -2; if (bitshiftR > 17) bitshiftR = 17;
    double gainL = 1.0;
    double gainR = 1.0;
    switch (bitshiftL)
    {
        case 17: gainL = 0.0; break;
        case 16: gainL = 0.0000152587890625; break;
        case 15: gainL = 0.000030517578125; break;
        case 14: gainL = 0.00006103515625; break;
        case 13: gainL = 0.0001220703125; break;
        case 12: gainL = 0.000244140625; break;
        case 11: gainL = 0.00048828125; break;
        case 10: gainL = 0.0009765625; break;
        case 9: gainL = 0.001953125; break;
        case 8: gainL = 0.00390625; break;
        case 7: gainL = 0.0078125; break;
        case 6: gainL = 0.015625; break;
        case 5: gainL = 0.03125; break;
        case 4: gainL = 0.0625; break;
        case 3: gainL = 0.125; break;
        case 2: gainL = 0.25; break;
        case 1: gainL = 0.5; break;
        case 0: gainL = 1.0; break;
        case -1: gainL = 2.0; break;
        case -2: gainL = 4.0; break;
        default: break;
    }
    switch (bitshiftR)
    {
        case 17: gainR = 0.0; break;
        case 16: gainR = 0.0000152587890625; break;
        case 15: gainR = 0.000030517578125; break;
        case 14: gainR = 0.00006103515625; break;
        case 13: gainR = 0.0001220703125; break;
        case 12: gainR = 0.000244140625; break;
        case 11: gainR = 0.00048828125; break;
        case 10: gainR = 0.0009765625; break;
        case 9: gainR = 0.001953125; break;
        case 8: gainR = 0.00390625; break;
        case 7: gainR = 0.0078125; break;
        case 6: gainR = 0.015625; break;
        case 5: gainR = 0.03125; break;
        case 4: gainR = 0.0625; break;
        case 3: gainR = 0.125; break;
        case 2: gainR = 0.25; break;
        case 1: gainR = 0.5; break;
        case 0: gainR = 1.0; break;
        case -1: gainR = 2.0; break;
        case -2: gainR = 4.0; break;
        default: break;
    }
    double temp;

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;

        temp = inputSampleL;
        inputSampleL = (inputSampleL + avgAL) * 0.5; avgAL = temp;
        temp = inputSampleR;
        inputSampleR = (inputSampleR + avgAR) * 0.5; avgAR = temp;

        inputSampleL *= gainL;
        inputSampleR *= gainR;

        if (inputSampleL > 2.8) inputSampleL = 2.8;
        if (inputSampleL < -2.8) inputSampleL = -2.8;
        if (inputSampleL > 0.0) inputSampleL = (inputSampleL * 2.0) / (3.0 - inputSampleL);
        else inputSampleL = -(inputSampleL * -2.0) / (3.0 + inputSampleL);
        if (inputSampleR > 2.8) inputSampleR = 2.8;
        if (inputSampleR < -2.8) inputSampleR = -2.8;
        if (inputSampleR > 0.0) inputSampleR = (inputSampleR * 2.0) / (3.0 - inputSampleR);
        else inputSampleR = -(inputSampleR * -2.0) / (3.0 + inputSampleR);
        //BigFastArcSin output stage.

        temp = inputSampleL;
        inputSampleL = (inputSampleL + avgBL) * 0.5; avgBL = temp;
        temp = inputSampleR;
        inputSampleR = (inputSampleR + avgBR) * 0.5; avgBR = temp;

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
