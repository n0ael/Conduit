/* ========================================
 *  PointyGuitar - PointyGuitar.cpp
 *  Copyright (c) Airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/PointyGuitar —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Der fpd-Dither-Add hängt an ditherOn(); der Xorshift läuft immer weiter
 *  (entspricht dem processDoubleReplacing-Pfad des Originals).
 * ======================================== */

#include "DSP/Airwindows/Plugins/PointyGuitar.h"

#include <cmath>
#include <cstdint>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "drive",   "Drive",   0.6f },
        { "presnce", "Presnce", 0.5f },
        { "high",    "High",    0.8f },
        { "mid",     "Mid",     0.6f },
        { "low",     "Low",     0.4f },
        { "sub",     "Sub",     0.5f },
        { "hspeakr", "HSpeakr", 0.7f },
        { "lspeakr", "LSpeakr", 0.4f },
        { "gate",    "Gate",    0.0f },
        { "output",  "Output",  0.7f },
    };
}

PointyGuitar::PointyGuitar() noexcept
    : AirwindowsPlugin ("pointyguitar", "PointyGuitar", kParameters)
{
}

void PointyGuitar::resetState() noexcept
{
    for (auto& row : angSL) for (auto& v : row) v = 0.0;
    for (auto& row : angAL) for (auto& v : row) v = 0.0;
    for (auto& row : angSR) for (auto& v : row) v = 0.0;
    for (auto& row : angAR) for (auto& v : row) v = 0.0;
    for (auto& v : iirHPositionL) v = 0.0;
    for (auto& v : iirHAngleL) v = 0.0;
    for (auto& v : iirBPositionL) v = 0.0;
    for (auto& v : iirBAngleL) v = 0.0;
    for (auto& v : iirHPositionR) v = 0.0;
    for (auto& v : iirHAngleR) v = 0.0;
    for (auto& v : iirBPositionR) v = 0.0;
    for (auto& v : iirBAngleR) v = 0.0;
    for (auto& v : angG) v = 0.0;

    WasNegativeL = false;
    ZeroCrossL = 0;
    gaterollerL = 0.0;
    gateL = 0.0;

    WasNegativeR = false;
    ZeroCrossR = 0;
    gaterollerR = 0.0;
    gateR = 0.0;
}

void PointyGuitar::processStereo (const float* in1, const float* in2,
                                  float* out1, float* out2, int sampleFrames) noexcept
{
    const float A = param (kParamA);
    const float B = param (kParamB);
    const float C = param (kParamC);
    const float D = param (kParamD);
    const float E = param (kParamE);
    const float F = param (kParamF);
    const float G = param (kParamG);
    const float H = param (kParamH);
    const float I = param (kParamI);
    const float J = param (kParamJ);

    double overallscale = 1.0;
    overallscale /= 44100.0;
    overallscale *= getSampleRate();

    double drive = A + 0.618033988749894;
    angG[0] = std::sqrt (B * 2.0);
    angG[2] = std::sqrt (C * 2.0);
    angG[4] = std::sqrt (D * 2.0);
    angG[6] = std::sqrt (E * 2.0);
    angG[8] = std::sqrt (F * 2.0);
    angG[1] = (angG[0] + angG[2]) * 0.5;
    angG[3] = (angG[2] + angG[4]) * 0.5;
    angG[5] = (angG[4] + angG[6]) * 0.5;
    angG[7] = (angG[6] + angG[8]) * 0.5;
    angG[9] = angG[8];
    int poles = (int) (drive * 10.0);
    double hFreq = std::pow (G, overallscale);
    double lFreq = std::pow (H, overallscale + 3.0);
    //begin Gate
    double onthreshold = (std::pow (I, 3) / 3) + 0.00018;
    double offthreshold = onthreshold * 1.1;
    double release = 0.028331119964586;
    double absmax = 220.9;
    //end Gate
    double output = J;

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;

        //begin Gate
        if (inputSampleL > 0.0)
        {
            if (WasNegativeL == true) ZeroCrossL = (int) (absmax * 0.3);
            WasNegativeL = false;
        }
        else
        {
            ZeroCrossL += 1; WasNegativeL = true;
        }
        if (ZeroCrossL > absmax) ZeroCrossL = (int) absmax;
        if (gateL == 0.0)
        {
            //if gateL is totally silent
            if (std::fabs (inputSampleL) > onthreshold)
            {
                if (gaterollerL == 0.0) gaterollerL = ZeroCrossL;
                else gaterollerL -= release;
                // trigger from total silence only- if we're active then signal must clear offthreshold
            }
            else gaterollerL -= release;
        }
        else
        {
            //gateL is not silent but closing
            if (std::fabs (inputSampleL) > offthreshold)
            {
                if (gaterollerL < ZeroCrossL) gaterollerL = ZeroCrossL;
                else gaterollerL -= release;
                //always trigger if gateL is over offthreshold, otherwise close anyway
            }
            else gaterollerL -= release;
        }
        if (gaterollerL < 0.0) gaterollerL = 0.0;

        for (int x = 0; x < poles; x++)
        {
            double fr = 0.9 / overallscale;
            double band = inputSampleL; inputSampleL = 0.0;
            for (int y = 0; y < 9; y++)
            {
                angAL[x][y] = (angAL[x][y] * (1.0 - fr)) + ((band - angSL[x][y]) * fr);
                double temp = band; band = ((angSL[x][y] + (angAL[x][y] * fr)) * (1.0 - fr)) + (band * fr);
                angSL[x][y] = ((angSL[x][y] + (angAL[x][y] * fr)) * (1.0 - fr)) + (band * fr);
                inputSampleL += ((temp - band) * angG[y]);
                fr *= 0.618033988749894;
            }
            inputSampleL += (band * angG[9]);
            inputSampleL *= drive;
            inputSampleL -= std::fmin (std::fmax ((inputSampleL * (std::fabs (inputSampleL) * 0.654) * (std::fabs (inputSampleL) * 0.654)), -1.0), 1.0);
        }

        if (gaterollerL < 1.0)
        {
            gateL = gaterollerL;
            double bridgerectifier = 1 - std::cos (std::fabs (inputSampleL));
            if (inputSampleL > 0) inputSampleL = (inputSampleL * gateL) + (bridgerectifier * (1.0 - gateL));
            else inputSampleL = (inputSampleL * gateL) - (bridgerectifier * (1.0 - gateL));
            if (gateL == 0.0) inputSampleL = 0.0;
        }
        else gateL = 1.0;
        //end Gate

        double lowSample = inputSampleL;
        for (int count = 0; count < (int) (3.0 + (lFreq * 32.0)); count++)
        {
            iirBAngleL[count] = (iirBAngleL[count] * (1.0 - lFreq)) + ((lowSample - iirBPositionL[count]) * lFreq);
            lowSample = ((iirBPositionL[count] + (iirBAngleL[count] * lFreq)) * (1.0 - lFreq)) + (lowSample * lFreq);
            iirBPositionL[count] = ((iirBPositionL[count] + (iirBAngleL[count] * lFreq)) * (1.0 - lFreq)) + (lowSample * lFreq);
            inputSampleL -= (lowSample * (1.0 / (3.0 + (lFreq * 32.0))));
        }

        for (int count = 0; count < (int) (3.0 + (hFreq * 32.0)); count++)
        {
            iirHAngleL[count] = (iirHAngleL[count] * (1.0 - hFreq)) + ((inputSampleL - iirHPositionL[count]) * hFreq);
            inputSampleL = ((iirHPositionL[count] + (iirHAngleL[count] * hFreq)) * (1.0 - hFreq)) + (inputSampleL * hFreq);
            iirHPositionL[count] = ((iirHPositionL[count] + (iirHAngleL[count] * hFreq)) * (1.0 - hFreq)) + (inputSampleL * hFreq);
        } //the lowpass
        inputSampleL *= output;

        //begin Gate
        if (inputSampleR > 0.0)
        {
            if (WasNegativeR == true) ZeroCrossR = (int) (absmax * 0.3);
            WasNegativeR = false;
        }
        else
        {
            ZeroCrossR += 1; WasNegativeR = true;
        }
        if (ZeroCrossR > absmax) ZeroCrossR = (int) absmax;
        if (gateR == 0.0)
        {
            //if gateR is totally silent
            if (std::fabs (inputSampleR) > onthreshold)
            {
                if (gaterollerR == 0.0) gaterollerR = ZeroCrossR;
                else gaterollerR -= release;
                // trigger from total silence only- if we're active then signal must clear offthreshold
            }
            else gaterollerR -= release;
        }
        else
        {
            //gateR is not silent but closing
            if (std::fabs (inputSampleR) > offthreshold)
            {
                if (gaterollerR < ZeroCrossR) gaterollerR = ZeroCrossR;
                else gaterollerR -= release;
                //always trigger if gateR is over offthreshold, otherwise close anyway
            }
            else gaterollerR -= release;
        }
        if (gaterollerR < 0.0) gaterollerR = 0.0;

        for (int x = 0; x < poles; x++)
        {
            double fr = 0.9 / overallscale;
            double band = inputSampleR; inputSampleR = 0.0;
            for (int y = 0; y < 9; y++)
            {
                angAR[x][y] = (angAR[x][y] * (1.0 - fr)) + ((band - angSR[x][y]) * fr);
                double temp = band; band = ((angSR[x][y] + (angAR[x][y] * fr)) * (1.0 - fr)) + (band * fr);
                angSR[x][y] = ((angSR[x][y] + (angAR[x][y] * fr)) * (1.0 - fr)) + (band * fr);
                inputSampleR += ((temp - band) * angG[y]);
                fr *= 0.618033988749894;
            }
            inputSampleR += (band * angG[9]);
            inputSampleR *= drive;
            inputSampleR -= std::fmin (std::fmax ((inputSampleR * (std::fabs (inputSampleR) * 0.654) * (std::fabs (inputSampleR) * 0.654)), -1.0), 1.0);
        }

        if (gaterollerR < 1.0)
        {
            gateR = gaterollerR;
            double bridgerectifier = 1 - std::cos (std::fabs (inputSampleR));
            if (inputSampleR > 0) inputSampleR = (inputSampleR * gateR) + (bridgerectifier * (1.0 - gateR));
            else inputSampleR = (inputSampleR * gateR) - (bridgerectifier * (1.0 - gateR));
            if (gateR == 0.0) inputSampleR = 0.0;
        }
        else gateR = 1.0;
        //end Gate

        lowSample = inputSampleR;
        for (int count = 0; count < (int) (3.0 + (lFreq * 32.0)); count++)
        {
            iirBAngleR[count] = (iirBAngleR[count] * (1.0 - lFreq)) + ((lowSample - iirBPositionR[count]) * lFreq);
            lowSample = ((iirBPositionR[count] + (iirBAngleR[count] * lFreq)) * (1.0 - lFreq)) + (lowSample * lFreq);
            iirBPositionR[count] = ((iirBPositionR[count] + (iirBAngleR[count] * lFreq)) * (1.0 - lFreq)) + (lowSample * lFreq);
            inputSampleR -= (lowSample * (1.0 / (3.0 + (lFreq * 32.0))));
        }

        for (int count = 0; count < (int) (3.0 + (hFreq * 32.0)); count++)
        {
            iirHAngleR[count] = (iirHAngleR[count] * (1.0 - hFreq)) + ((inputSampleR - iirHPositionR[count]) * hFreq);
            inputSampleR = ((iirHPositionR[count] + (iirHAngleR[count] * hFreq)) * (1.0 - hFreq)) + (inputSampleR * hFreq);
            iirHPositionR[count] = ((iirHPositionR[count] + (iirHAngleR[count] * hFreq)) * (1.0 - hFreq)) + (inputSampleR * hFreq);
        } //the lowpass
        inputSampleR *= output;

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
