/* ========================================
 *  Inflamer - Inflamer.cpp
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Inflamer —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter
 *  (entspricht dem processDoubleReplacing-Pfad des Originals).
 * ======================================== */

#include "DSP/Airwindows/Plugins/Inflamer.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "drive",  "Drive",  0.5f },
        { "curve",  "Curve",  0.5f },
        { "effect", "Effect", 1.0f },
    };

    // Original: switch-Tabelle 2^-n / 2^n über gainBits -2..17, gleich für
    // Input-Trim (gain), negacurve (bitshiftL) und curve (bitshiftR).
    double bitGain (int bits) noexcept
    {
        switch (bits)
        {
            case 17: return 0.0;
            case 16: return 0.0000152587890625;
            case 15: return 0.000030517578125;
            case 14: return 0.00006103515625;
            case 13: return 0.0001220703125;
            case 12: return 0.000244140625;
            case 11: return 0.00048828125;
            case 10: return 0.0009765625;
            case 9:  return 0.001953125;
            case 8:  return 0.00390625;
            case 7:  return 0.0078125;
            case 6:  return 0.015625;
            case 5:  return 0.03125;
            case 4:  return 0.0625;
            case 3:  return 0.125;
            case 2:  return 0.25;
            case 1:  return 0.5;
            case 0:  return 1.0;
            case -1: return 2.0;
            case -2: return 4.0;
            default: return 1.0;
        }
    }
}

Inflamer::Inflamer() noexcept
    : AirwindowsPlugin ("inflamer", "Inflamer", kParameters)
{
}

void Inflamer::resetState() noexcept
{
    // zustandslos — nur fpdL/fpdR, die setzt die Basis in prepare()
}

void Inflamer::processStereo (const float* in1, const float* in2,
                              float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);
    const float B = param (kParamB);
    const float C = param (kParamC);

    double gainControl = (A * 0.5) + 0.05; //0.0 to 1.0
    int gainBits = 20; //start beyond maximum attenuation
    if (gainControl > 0.0) gainBits = (int) std::floor (1.0 / gainControl) - 2;
    if (gainBits < -2) gainBits = -2;
    if (gainBits > 17) gainBits = 17;
    double gain = bitGain (gainBits); //now we have our input trim

    int bitshiftL = 1;
    int bitshiftR = 1;
    double panControl = (B * 2.0) - 1.0; //-1.0 to 1.0
    double panAttenuation = (1.0 - std::fabs (panControl));
    int panBits = 20; //start centered
    if (panAttenuation > 0.0) panBits = (int) std::floor (1.0 / panAttenuation);
    if (panControl > 0.25) bitshiftL += panBits;
    if (panControl < -0.25) bitshiftR += panBits;
    if (bitshiftL < -2) bitshiftL = -2;
    if (bitshiftL > 17) bitshiftL = 17;
    if (bitshiftR < -2) bitshiftR = -2;
    if (bitshiftR > 17) bitshiftR = 17;
    double negacurve = bitGain (bitshiftL);
    double curve = bitGain (bitshiftR);
    double effectOut = C;

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;
        double drySampleL = inputSampleL;
        double drySampleR = inputSampleR;

        inputSampleL *= gain; //input trim
        double inflamerPlus = (inputSampleL * 2.0) - std::pow (inputSampleL, 2); //+50, very much just second harmonic
        double inflamerMinus = inputSampleL + (std::pow (inputSampleL, 3) * 0.25) - ((std::pow (inputSampleL, 2) + std::pow (inputSampleL, 4)) * 0.0625); //-50
        inputSampleL = (inflamerPlus * curve) + (inflamerMinus * negacurve);
        inputSampleL = (inputSampleL * effectOut) + (drySampleL * (1.0 - effectOut));

        inputSampleR *= gain; //input trim
        inflamerPlus = (inputSampleR * 2.0) - std::pow (inputSampleR, 2); //+50, very much just second harmonic
        inflamerMinus = inputSampleR + (std::pow (inputSampleR, 3) * 0.25) - ((std::pow (inputSampleR, 2) + std::pow (inputSampleR, 4)) * 0.0625); //-50
        inputSampleR = (inflamerPlus * curve) + (inflamerMinus * negacurve);
        inputSampleR = (inputSampleR * effectOut) + (drySampleR * (1.0 - effectOut));

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
