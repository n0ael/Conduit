/* ========================================
 *  TapeDelay2 - TapeDelay2.cpp
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/TapeDelay2 —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter
 *  (entspricht dem processDoubleReplacing-Pfad des Originals).
 * ======================================== */

#include "DSP/Airwindows/Plugins/TapeDelay2.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "time",    "Time",    1.0f },
        { "regen",   "Regen",   0.0f },
        { "freq",    "Freq",    0.5f },
        { "reso",    "Reso",    0.0f },
        { "flutter", "Flutter", 0.0f },
        { "dry_wet", "Dry/Wet", 1.0f },
    };
}

TapeDelay2::TapeDelay2() noexcept
    : AirwindowsPlugin ("tapedelay2", "TapeDelay2", kParameters)
{
}

void TapeDelay2::resetState() noexcept
{
    for (auto& v : dL) v = 0.0;
    for (auto& v : dR) v = 0.0;
    prevSampleL = 0.0; delayL = 0.0; sweepL = 0.0;
    prevSampleR = 0.0; delayR = 0.0; sweepR = 0.0;
    for (auto& v : regenFilterL) v = 0.0;
    for (auto& v : outFilterL) v = 0.0;
    for (auto& v : lastRefL) v = 0.0;
    for (auto& v : regenFilterR) v = 0.0;
    for (auto& v : outFilterR) v = 0.0;
    for (auto& v : lastRefR) v = 0.0;
    cycle = 0;
}

void TapeDelay2::processStereo (const float* in1, const float* in2,
                                float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);
    const float B = param (kParamB);
    const float C = param (kParamC);
    const float D = param (kParamD);
    const float E = param (kParamE);
    const float F = param (kParamF);

    double overallscale = 1.0;
    overallscale /= 44100.0;
    overallscale *= getSampleRate();

    int cycleEnd = (int) std::floor (overallscale);
    if (cycleEnd < 1) cycleEnd = 1;
    if (cycleEnd > 4) cycleEnd = 4;
    //this is going to be 2 for 88.1 or 96k, 3 for silly people, 4 for 176 or 192k
    if (cycle > cycleEnd - 1) cycle = cycleEnd - 1; //sanity check

    double baseSpeed = (std::pow (A, 4) * 25.0) + 1.0;
    double feedback = std::pow (B, 2);

    //[0] is frequency: 0.000001 to 0.499999 is near-zero to near-Nyquist
    //[1] is resonance, 0.7071 is Butterworth. Also can't be zero
    regenFilterL[0] = regenFilterR[0] = ((std::pow (C, 3) * 0.4) + 0.0001);
    regenFilterL[1] = regenFilterR[1] = std::pow (D, 2) + 0.01; //resonance
    double K = std::tan (juce::MathConstants<double>::pi * regenFilterR[0]);
    double norm = 1.0 / (1.0 + K / regenFilterR[1] + K * K);
    regenFilterL[2] = regenFilterR[2] = K / regenFilterR[1] * norm;
    regenFilterL[4] = regenFilterR[4] = -regenFilterR[2];
    regenFilterL[5] = regenFilterR[5] = 2.0 * (K * K - 1.0) * norm;
    regenFilterL[6] = regenFilterR[6] = (1.0 - K / regenFilterR[1] + K * K) * norm;

    //[0] is frequency: 0.000001 to 0.499999 is near-zero to near-Nyquist
    //[1] is resonance, 0.7071 is Butterworth. Also can't be zero
    outFilterL[0] = outFilterR[0] = regenFilterR[0];
    outFilterL[1] = outFilterR[1] = regenFilterR[1] * 1.618033988749894848204586; //resonance
    K = std::tan (juce::MathConstants<double>::pi * outFilterR[0]);
    norm = 1.0 / (1.0 + K / outFilterR[1] + K * K);
    outFilterL[2] = outFilterR[2] = K / outFilterR[1] * norm;
    outFilterL[4] = outFilterR[4] = -outFilterR[2];
    outFilterL[5] = outFilterR[5] = 2.0 * (K * K - 1.0) * norm;
    outFilterL[6] = outFilterR[6] = (1.0 - K / outFilterR[1] + K * K) * norm;

    double vibSpeed = std::pow (E, 5) * baseSpeed * ((regenFilterR[0] * 0.09) + 0.025); //0.05
    double wet = F * 2.0;
    double dry = 2.0 - wet;
    if (wet > 1.0) wet = 1.0;
    if (wet < 0.0) wet = 0.0;
    if (dry > 1.0) dry = 1.0;
    if (dry < 0.0) dry = 0.0;
    //this echo makes 50% full dry AND full wet, not crossfaded.
    //that's so it can be on submixes without cutting back dry channel when adjusted:
    //unless you go super heavy, you are only adjusting the added echo loudness.

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;
        double drySampleL = inputSampleL;
        double drySampleR = inputSampleR;

        cycle++;
        if (cycle == cycleEnd)
        {
            double speedL = baseSpeed + (vibSpeed * (std::sin (sweepL) + 1.0));
            double speedR = baseSpeed + (vibSpeed * (std::sin (sweepR) + 1.0));
            sweepL += (0.05 * inputSampleL * inputSampleL); if (sweepL > 6.283185307179586) sweepL -= 6.283185307179586;
            sweepR += (0.05 * inputSampleR * inputSampleR); if (sweepR > 6.283185307179586) sweepR -= 6.283185307179586;

            //begin left channel
            int pos = (int) std::floor (delayL);
            double newSample = inputSampleL + dL[pos] * feedback;
            double tempSample = (newSample * regenFilterL[2]) + regenFilterL[7];
            regenFilterL[7] = -(tempSample * regenFilterL[5]) + regenFilterL[8];
            regenFilterL[8] = (newSample * regenFilterL[4]) - (tempSample * regenFilterL[6]);
            newSample = tempSample;

            delayL -= speedL; if (delayL < 0) delayL += 88200.0;
            double increment = (newSample - prevSampleL) / speedL;
            dL[pos] = prevSampleL;
            while (pos != (int) std::floor (delayL))
            {
                dL[pos] = prevSampleL;
                prevSampleL += increment;
                pos--; if (pos < 0) pos += 88200;
            }
            prevSampleL = newSample;
            pos = (int) std::floor (delayL); inputSampleL = dL[pos];
            tempSample = (inputSampleL * outFilterL[2]) + outFilterL[7];
            outFilterL[7] = -(tempSample * outFilterL[5]) + outFilterL[8];
            outFilterL[8] = (inputSampleL * outFilterL[4]) - (tempSample * outFilterL[6]);
            inputSampleL = tempSample;
            //end left channel
            //begin right channel
            pos = (int) std::floor (delayR);
            newSample = inputSampleR + dR[pos] * feedback;
            tempSample = (newSample * regenFilterR[2]) + regenFilterR[7];
            regenFilterR[7] = -(tempSample * regenFilterR[5]) + regenFilterR[8];
            regenFilterR[8] = (newSample * regenFilterR[4]) - (tempSample * regenFilterR[6]);
            newSample = tempSample;

            delayR -= speedR; if (delayR < 0) delayR += 88200.0;
            increment = (newSample - prevSampleR) / speedR;
            dR[pos] = prevSampleR;
            while (pos != (int) std::floor (delayR))
            {
                dR[pos] = prevSampleR;
                prevSampleR += increment;
                pos--; if (pos < 0) pos += 88200;
            }
            prevSampleR = newSample;
            pos = (int) std::floor (delayR); inputSampleR = dR[pos];
            tempSample = (inputSampleR * outFilterR[2]) + outFilterR[7];
            outFilterR[7] = -(tempSample * outFilterR[5]) + outFilterR[8];
            outFilterR[8] = (inputSampleR * outFilterR[4]) - (tempSample * outFilterR[6]);
            inputSampleR = tempSample;
            //end right channel

            if (cycleEnd == 4)
            {
                lastRefL[0] = lastRefL[4]; //start from previous last
                lastRefL[2] = (lastRefL[0] + inputSampleL) / 2; //half
                lastRefL[1] = (lastRefL[0] + lastRefL[2]) / 2; //one quarter
                lastRefL[3] = (lastRefL[2] + inputSampleL) / 2; //three quarters
                lastRefL[4] = inputSampleL; //full
                lastRefR[0] = lastRefR[4]; //start from previous last
                lastRefR[2] = (lastRefR[0] + inputSampleR) / 2; //half
                lastRefR[1] = (lastRefR[0] + lastRefR[2]) / 2; //one quarter
                lastRefR[3] = (lastRefR[2] + inputSampleR) / 2; //three quarters
                lastRefR[4] = inputSampleR; //full
            }
            if (cycleEnd == 3)
            {
                lastRefL[0] = lastRefL[3]; //start from previous last
                lastRefL[2] = (lastRefL[0] + lastRefL[0] + inputSampleL) / 3; //third
                lastRefL[1] = (lastRefL[0] + inputSampleL + inputSampleL) / 3; //two thirds
                lastRefL[3] = inputSampleL; //full
                lastRefR[0] = lastRefR[3]; //start from previous last
                lastRefR[2] = (lastRefR[0] + lastRefR[0] + inputSampleR) / 3; //third
                lastRefR[1] = (lastRefR[0] + inputSampleR + inputSampleR) / 3; //two thirds
                lastRefR[3] = inputSampleR; //full
            }
            if (cycleEnd == 2)
            {
                lastRefL[0] = lastRefL[2]; //start from previous last
                lastRefL[1] = (lastRefL[0] + inputSampleL) / 2; //half
                lastRefL[2] = inputSampleL; //full
                lastRefR[0] = lastRefR[2]; //start from previous last
                lastRefR[1] = (lastRefR[0] + inputSampleR) / 2; //half
                lastRefR[2] = inputSampleR; //full
            }
            if (cycleEnd == 1)
            {
                lastRefL[0] = inputSampleL;
                lastRefR[0] = inputSampleR;
            }
            cycle = 0; //reset
            inputSampleL = lastRefL[cycle];
            inputSampleR = lastRefR[cycle];
        }
        else
        {
            inputSampleL = lastRefL[cycle];
            inputSampleR = lastRefR[cycle];
            //we are going through our references now
        }
        switch (cycleEnd) //multi-pole average using lastRef[] variables
        {
            case 4:
                lastRefL[8] = inputSampleL; inputSampleL = (inputSampleL + lastRefL[7]) * 0.5;
                lastRefL[7] = lastRefL[8]; //continue, do not break
                lastRefR[8] = inputSampleR; inputSampleR = (inputSampleR + lastRefR[7]) * 0.5;
                lastRefR[7] = lastRefR[8]; //continue, do not break
                [[fallthrough]];
            case 3:
                lastRefL[8] = inputSampleL; inputSampleL = (inputSampleL + lastRefL[6]) * 0.5;
                lastRefL[6] = lastRefL[8]; //continue, do not break
                lastRefR[8] = inputSampleR; inputSampleR = (inputSampleR + lastRefR[6]) * 0.5;
                lastRefR[6] = lastRefR[8]; //continue, do not break
                [[fallthrough]];
            case 2:
                lastRefL[8] = inputSampleL; inputSampleL = (inputSampleL + lastRefL[5]) * 0.5;
                lastRefL[5] = lastRefL[8]; //continue, do not break
                lastRefR[8] = inputSampleR; inputSampleR = (inputSampleR + lastRefR[5]) * 0.5;
                lastRefR[5] = lastRefR[8]; //continue, do not break
                [[fallthrough]];
            case 1:
                break; //no further averaging
            default:
                break;
        }

        if (wet < 1.0) { inputSampleL *= wet; inputSampleR *= wet; }
        if (dry < 1.0) { drySampleL *= dry; drySampleR *= dry; }
        inputSampleL += drySampleL; inputSampleR += drySampleR;
        //this is our submix echo dry/wet: 0.5 is BOTH at FULL VOLUME
        //purpose is that, if you're adding echo, you're not altering other balances

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
