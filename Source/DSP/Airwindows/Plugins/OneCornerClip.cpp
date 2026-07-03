/* ========================================
 *  OneCornerClip - OneCornerClip.cpp
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/OneCornerClip —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter
 *  (entspricht dem processDoubleReplacing-Pfad des Originals).
 * ======================================== */

#include "DSP/Airwindows/Plugins/OneCornerClip.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "input",   "Input",   0.33333333333333333f },
        { "pos_thr", "Pos Thr", 0.966f },
        { "neg_thr", "Neg Thr", 0.966f },
        { "voicing", "Voicing", 0.618f },
        { "dry_wet", "Dry/Wet", 1.0f },
    };
}

OneCornerClip::OneCornerClip() noexcept
    : AirwindowsPlugin ("onecornerclip", "OneCornerClip", kParameters)
{
}

void OneCornerClip::resetState() noexcept
{
    lastSampleL = 0.0;
    limitPosL = 0.0;
    limitNegL = 0.0;
    lastSampleR = 0.0;
    limitPosR = 0.0;
    limitNegR = 0.0;
}

void OneCornerClip::processStereo (const float* in1, const float* in2,
                                   float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);
    const float B = param (kParamB);
    const float C = param (kParamC);
    const float D = param (kParamD);
    const float E = param (kParamE);

    double overallscale = 1.0;
    overallscale /= 44100.0;
    overallscale *= getSampleRate();

    double inputGain = std::pow (10.0, (((A * 36.0) - 12.0) / 20.0));
    double posThreshold = B;
    double posTargetL = posThreshold;
    double posTargetR = posThreshold;
    double negThreshold = -C;
    double negTargetL = negThreshold;
    double negTargetR = negThreshold;
    double voicing = D;
    if (voicing == 0.618) voicing = 0.618033988749894848204586;
    //special case: we will do a perfect golden ratio as the default 0.618
    //just 'cos magic universality sauce (seriously, it seems a sweetspot)
    if (overallscale > 0.0) voicing /= overallscale;
    //translate to desired sample rate, 44.1K is the base
    if (voicing < 0.0) voicing = 0.0;
    if (voicing > 1.0) voicing = 1.0;
    //some insanity checking
    double inverseHardness = 1.0 - voicing;
    bool clipEngage = false;

    double wet = E;
    //removed extra dry variable
    double drySampleL;
    double drySampleR;
    double inputSampleL;
    double inputSampleR;

    while (--sampleFrames >= 0)
    {
        inputSampleL = *in1;
        inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;
        drySampleL = inputSampleL;
        drySampleR = inputSampleR;

        if (inputGain != 1.0)
        {
            inputSampleL *= inputGain;
            inputSampleR *= inputGain;
            clipEngage = true;
            //if we are altering gain we will always process
        }
        else
        {
            clipEngage = false;
            //if we are not touching gain, we will bypass unless
            //a clip is actively being softened.
        }

        if (inputSampleL > posTargetL)
        {
            inputSampleL = (lastSampleL * voicing) + (posThreshold * inverseHardness);
            posTargetL = inputSampleL;
            clipEngage = true;
        }
        else
        {
            posTargetL = posThreshold;
        }

        if (inputSampleR > posTargetR)
        {
            inputSampleR = (lastSampleR * voicing) + (posThreshold * inverseHardness);
            posTargetR = inputSampleR;
            clipEngage = true;
        }
        else
        {
            posTargetR = posThreshold;
        }

        if (inputSampleL < negTargetL)
        {
            inputSampleL = (lastSampleL * voicing) + (negThreshold * inverseHardness);
            negTargetL = inputSampleL;
            clipEngage = true;
        }
        else
        {
            negTargetL = negThreshold;
        }

        if (inputSampleR < negTargetR)
        {
            inputSampleR = (lastSampleR * voicing) + (negThreshold * inverseHardness);
            negTargetR = inputSampleR;
            clipEngage = true;
        }
        else
        {
            negTargetR = negThreshold;
        }

        lastSampleL = inputSampleL;
        lastSampleR = inputSampleR;

        if (wet != 1.0)
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

        if (clipEngage == false)
        {
            inputSampleL = *in1;
            inputSampleR = *in2;
        }
        //fall back to raw passthrough if at all possible

        *out1 = (float) inputSampleL;
        *out2 = (float) inputSampleR;

        ++in1;
        ++in2;
        ++out1;
        ++out2;
    }
}

} // namespace conduit::airwindows
