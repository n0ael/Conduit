/* ========================================
 *  Gatelope - Gatelope.cpp
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Gatelope —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter
 *  (entspricht dem processDoubleReplacing-Pfad des Originals).
 * ======================================== */

#include "DSP/Airwindows/Plugins/Gatelope.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "thresh",   "Thresh",  0.0f },
        { "treb_sus", "TrebSus", 1.0f },
        { "bass_sus", "BassSus", 0.5f },
        { "attack_s", "AttackS", 0.0f },
        { "dry_wet",  "Dry/Wet", 1.0f },
    };
}

Gatelope::Gatelope() noexcept
    : AirwindowsPlugin ("gatelope", "Gatelope", kParameters)
{
}

void Gatelope::resetState() noexcept
{
    iirLowpassAL = 0.0;
    iirLowpassBL = 0.0;
    iirHighpassAL = 0.0;
    iirHighpassBL = 0.0;
    iirLowpassAR = 0.0;
    iirLowpassBR = 0.0;
    iirHighpassAR = 0.0;
    iirHighpassBR = 0.0;
    treblefreq = 1.0;
    bassfreq = 0.0;
    flip = false;
}

void Gatelope::processStereo (const float* in1, const float* in2,
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
    //speed settings around release
    double threshold = std::pow (A, 2);
    //gain settings around threshold
    double trebledecay = std::pow (1.0 - B, 2) / 4196.0;
    double bassdecay = std::pow (1.0 - C, 2) / 8192.0;
    double slowAttack = (std::pow (D, 3) * 3) + 0.003;
    double wet = E;
    slowAttack /= overallscale;
    trebledecay /= overallscale;
    bassdecay /= overallscale;
    trebledecay += 1.0;
    bassdecay += 1.0;
    double attackSpeed;
    double highestSample;
    //this VST version comes from the AU, Gatelinked, because it's stereo.
    //if used on a mono track it'll act like the mono N to N

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;

        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;
        double drySampleL = inputSampleL;
        double drySampleR = inputSampleR;

        if (std::fabs (inputSampleL) > std::fabs (inputSampleR))
        {
            attackSpeed = slowAttack - (std::fabs (inputSampleL) * slowAttack * 0.5);
            highestSample = std::fabs (inputSampleL);
        }
        else
        {
            attackSpeed = slowAttack - (std::fabs (inputSampleR) * slowAttack * 0.5); //we're triggering off the highest amplitude
            highestSample = std::fabs (inputSampleR); //and making highestSample the abs() of that amplitude
        }

        if (attackSpeed < 0.0) attackSpeed = 0.0;
        //softening onset click depending on how hard we're getting it

        if (flip)
        {
            if (highestSample > threshold)
            {
                treblefreq += attackSpeed;
                if (treblefreq > 2.0) treblefreq = 2.0;
                bassfreq -= attackSpeed;
                bassfreq -= attackSpeed;
                if (bassfreq < 0.0) bassfreq = 0.0;
                iirLowpassAL = iirLowpassBL = inputSampleL;
                iirHighpassAL = iirHighpassBL = 0.0;
                iirLowpassAR = iirLowpassBR = inputSampleR;
                iirHighpassAR = iirHighpassBR = 0.0;
            }
            else
            {
                treblefreq -= bassfreq;
                treblefreq /= trebledecay;
                treblefreq += bassfreq;
                bassfreq -= treblefreq;
                bassfreq /= bassdecay;
                bassfreq += treblefreq;
            }

            if (treblefreq >= 1.0)
            {
                iirLowpassAL = inputSampleL;
                iirLowpassAR = inputSampleR;
            }
            else
            {
                iirLowpassAL = (iirLowpassAL * (1.0 - treblefreq)) + (inputSampleL * treblefreq);
                iirLowpassAR = (iirLowpassAR * (1.0 - treblefreq)) + (inputSampleR * treblefreq);
            }

            if (bassfreq > 1.0) bassfreq = 1.0;

            if (bassfreq > 0.0)
            {
                iirHighpassAL = (iirHighpassAL * (1.0 - bassfreq)) + (inputSampleL * bassfreq);
                iirHighpassAR = (iirHighpassAR * (1.0 - bassfreq)) + (inputSampleR * bassfreq);
            }
            else
            {
                iirHighpassAL = 0.0;
                iirHighpassAR = 0.0;
            }

            if (treblefreq > bassfreq)
            {
                inputSampleL = (iirLowpassAL - iirHighpassAL);
                inputSampleR = (iirLowpassAR - iirHighpassAR);
            }
            else
            {
                inputSampleL = 0.0;
                inputSampleR = 0.0;
            }
        }
        else
        {
            if (highestSample > threshold)
            {
                treblefreq += attackSpeed;
                if (treblefreq > 2.0) treblefreq = 2.0;
                bassfreq -= attackSpeed;
                bassfreq -= attackSpeed;
                if (bassfreq < 0.0) bassfreq = 0.0;
                iirLowpassAL = iirLowpassBL = inputSampleL;
                iirHighpassAL = iirHighpassBL = 0.0;
                iirLowpassAR = iirLowpassBR = inputSampleR;
                iirHighpassAR = iirHighpassBR = 0.0;
            }
            else
            {
                treblefreq -= bassfreq;
                treblefreq /= trebledecay;
                treblefreq += bassfreq;
                bassfreq -= treblefreq;
                bassfreq /= bassdecay;
                bassfreq += treblefreq;
            }

            if (treblefreq >= 1.0)
            {
                iirLowpassBL = inputSampleL;
                iirLowpassBR = inputSampleR;
            }
            else
            {
                iirLowpassBL = (iirLowpassBL * (1.0 - treblefreq)) + (inputSampleL * treblefreq);
                iirLowpassBR = (iirLowpassBR * (1.0 - treblefreq)) + (inputSampleR * treblefreq);
            }

            if (bassfreq > 1.0) bassfreq = 1.0;

            if (bassfreq > 0.0)
            {
                iirHighpassBL = (iirHighpassBL * (1.0 - bassfreq)) + (inputSampleL * bassfreq);
                iirHighpassBR = (iirHighpassBR * (1.0 - bassfreq)) + (inputSampleR * bassfreq);
            }
            else
            {
                iirHighpassBL = 0.0;
                iirHighpassBR = 0.0;
            }

            if (treblefreq > bassfreq)
            {
                inputSampleL = (iirLowpassBL - iirHighpassBL);
                inputSampleR = (iirLowpassBR - iirHighpassBR);
            }
            else
            {
                inputSampleL = 0.0;
                inputSampleR = 0.0;
            }
        }
        //done full gated envelope filtered effect
        inputSampleL = ((1 - wet) * drySampleL) + (wet * inputSampleL);
        inputSampleR = ((1 - wet) * drySampleR) + (wet * inputSampleR);
        //we're going to set up a dry/wet control instead of a min. threshold

        flip = !flip;

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
