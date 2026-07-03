/* ========================================
 *  ConsoleMCBuss - ConsoleMCBuss.cpp
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/ConsoleMCBuss —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter
 *  (entspricht dem processDoubleReplacing-Pfad des Originals).
 * ======================================== */

#include "DSP/Airwindows/Plugins/ConsoleMCBuss.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "master", "Master", 1.0f },
    };
}

ConsoleMCBuss::ConsoleMCBuss() noexcept
    : AirwindowsPlugin ("consolemcbuss", "ConsoleMCBuss", kParameters)
{
}

void ConsoleMCBuss::resetState() noexcept
{
    lastSinewL = lastSinewR = 0.0;
    subAL = subAR = subBL = subBR = subCL = subCR = subDL = subDR = 0.0;
    gainA = gainB = 1.0;
}

void ConsoleMCBuss::processStereo (const float* in1, const float* in2,
                                   float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kMasterParam);

    const int inFramesToProcess = sampleFrames; //Original erfasst sampleFrames vor der Dekrementierschleife
    double overallscale = 1.0;
    overallscale /= 44100.0;
    overallscale *= getSampleRate();

    gainA = gainB;
    gainB = std::sqrt ((double) A); //smoothed master fader from Z2 filters
    //this will be applied three times: this is to make the various tone alterations
    //hit differently at different master fader drive levels.
    //in particular, backing off the master fader tightens the super lows
    //but opens up the modified Sinew, because more of the attentuation happens before
    //you even get to slew clipping :) and if the fader is not active, it bypasses completely.

    double threshSinew = 0.5171104 / overallscale;
    double subTrim = 0.001 / overallscale;

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;

        double temp = (double) sampleFrames / (double) inFramesToProcess;
        double gain = (gainA * temp) + (gainB * (1.0 - temp));
        //setting up smoothed master fader

        //begin SubTight section
        double subSampleL = inputSampleL * subTrim;
        double subSampleR = inputSampleR * subTrim;

        double scale = 0.5 + std::fabs (subSampleL * 0.5);
        subSampleL = (subAL + (std::sin (subAL - subSampleL) * scale));
        subAL = subSampleL * scale;
        scale = 0.5 + std::fabs (subSampleR * 0.5);
        subSampleR = (subAR + (std::sin (subAR - subSampleR) * scale));
        subAR = subSampleR * scale;
        scale = 0.5 + std::fabs (subSampleL * 0.5);
        subSampleL = (subBL + (std::sin (subBL - subSampleL) * scale));
        subBL = subSampleL * scale;
        scale = 0.5 + std::fabs (subSampleR * 0.5);
        subSampleR = (subBR + (std::sin (subBR - subSampleR) * scale));
        subBR = subSampleR * scale;
        scale = 0.5 + std::fabs (subSampleL * 0.5);
        subSampleL = (subCL + (std::sin (subCL - subSampleL) * scale));
        subCL = subSampleL * scale;
        scale = 0.5 + std::fabs (subSampleR * 0.5);
        subSampleR = (subCR + (std::sin (subCR - subSampleR) * scale));
        subCR = subSampleR * scale;
        scale = 0.5 + std::fabs (subSampleL * 0.5);
        subSampleL = (subDL + (std::sin (subDL - subSampleL) * scale));
        subDL = subSampleL * scale;
        scale = 0.5 + std::fabs (subSampleR * 0.5);
        subSampleR = (subDR + (std::sin (subDR - subSampleR) * scale));
        subDR = subSampleR * scale;
        if (subSampleL > 0.25) subSampleL = 0.25;
        if (subSampleL < -0.25) subSampleL = -0.25;
        if (subSampleR > 0.25) subSampleR = 0.25;
        if (subSampleR < -0.25) subSampleR = -0.25;
        inputSampleL -= (subSampleL * 16.0);
        inputSampleR -= (subSampleR * 16.0);
        //end SubTight section

        if (gain < 1.0) {
            inputSampleL *= gain;
            inputSampleR *= gain;
        } //if using the master fader, we are going to attenuate three places.
        //subtight is always fully engaged: tighten response when restraining full console

        //begin Console7Buss which is the one we choose for ConsoleMC
        if (inputSampleL > 1.0) inputSampleL = 1.0;
        if (inputSampleL < -1.0) inputSampleL = -1.0;
        inputSampleL = ((std::asin (inputSampleL * std::fabs (inputSampleL)) / ((std::fabs (inputSampleL) == 0.0) ? 1 : std::fabs (inputSampleL))) * 0.618033988749894848204586) + (std::asin (inputSampleL) * 0.381966011250105);

        if (inputSampleR > 1.0) inputSampleR = 1.0;
        if (inputSampleR < -1.0) inputSampleR = -1.0;
        inputSampleR = ((std::asin (inputSampleR * std::fabs (inputSampleR)) / ((std::fabs (inputSampleR) == 0.0) ? 1 : std::fabs (inputSampleR))) * 0.618033988749894848204586) + (std::asin (inputSampleR) * 0.381966011250105);
        //this is an asin version of Spiral blended with regular asin ConsoleBuss.
        //It's blending between two different harmonics in the overtones of the algorithm

        if (gain < 1.0) {
            inputSampleL *= gain;
            inputSampleR *= gain;
        } //if using the master fader, we are going to attenuate three places.
        //after C7Buss but before EverySlew: allow highs to come out a bit more
        //when pulling back master fader. Less drive equals more open

        temp = inputSampleL;
        double clamp = inputSampleL - lastSinewL;
        if (lastSinewL > 1.0) lastSinewL = 1.0;
        if (lastSinewL < -1.0) lastSinewL = -1.0;
        double sinew = threshSinew * std::cos (lastSinewL);
        if (clamp > sinew) temp = lastSinewL + sinew;
        if (-clamp > sinew) temp = lastSinewL - sinew;
        inputSampleL = lastSinewL = temp;
        temp = inputSampleR;
        clamp = inputSampleR - lastSinewR;
        if (lastSinewR > 1.0) lastSinewR = 1.0;
        if (lastSinewR < -1.0) lastSinewR = -1.0;
        sinew = threshSinew * std::cos (lastSinewR);
        if (clamp > sinew) temp = lastSinewR + sinew;
        if (-clamp > sinew) temp = lastSinewR - sinew;
        inputSampleR = lastSinewR = temp;

        if (gain < 1.0) {
            inputSampleL *= gain;
            inputSampleR *= gain;
        } //if using the master fader, we are going to attenuate three places.
        //after EverySlew fades the total output sound: least change in tone here.

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
