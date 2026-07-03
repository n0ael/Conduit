/* ========================================
 *  VerbTiny - VerbTinyProc.cpp
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus plugins/LinuxVST/src/VerbTiny (github.com/airwindows/airwindows) —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#include "DSP/Airwindows/Plugins/VerbTiny.h"

#include <cmath>
#include <cstdint>
#include <algorithm>

namespace conduit::airwindows
{

namespace
{
    constexpr ParameterInfo kParameters[] = {
        { "replace",  "Replace",  0.5f },
        { "derez",    "Derez",    1.0f },
        { "filter",   "Filter",   1.0f },
        { "wider",    "Wider",    0.0f },
        { "dry_wet",  "Dry/Wet",  1.0f },
    };
}

VerbTiny::VerbTiny() noexcept
    : AirwindowsPlugin ("verbtiny", "VerbTiny", kParameters)
{
}

void VerbTiny::resetState() noexcept
{
    std::fill (std::begin (a4AL), std::end (a4AL), 0.0); std::fill (std::begin (a4AR), std::end (a4AR), 0.0);
    std::fill (std::begin (a4BL), std::end (a4BL), 0.0); std::fill (std::begin (a4BR), std::end (a4BR), 0.0);
    std::fill (std::begin (a4CL), std::end (a4CL), 0.0); std::fill (std::begin (a4CR), std::end (a4CR), 0.0);
    std::fill (std::begin (a4DL), std::end (a4DL), 0.0); std::fill (std::begin (a4DR), std::end (a4DR), 0.0);
    std::fill (std::begin (a4EL), std::end (a4EL), 0.0); std::fill (std::begin (a4ER), std::end (a4ER), 0.0);
    std::fill (std::begin (a4FL), std::end (a4FL), 0.0); std::fill (std::begin (a4FR), std::end (a4FR), 0.0);
    std::fill (std::begin (a4GL), std::end (a4GL), 0.0); std::fill (std::begin (a4GR), std::end (a4GR), 0.0);
    std::fill (std::begin (a4HL), std::end (a4HL), 0.0); std::fill (std::begin (a4HR), std::end (a4HR), 0.0);
    std::fill (std::begin (a4IL), std::end (a4IL), 0.0); std::fill (std::begin (a4IR), std::end (a4IR), 0.0);
    std::fill (std::begin (a4JL), std::end (a4JL), 0.0); std::fill (std::begin (a4JR), std::end (a4JR), 0.0);
    std::fill (std::begin (a4KL), std::end (a4KL), 0.0); std::fill (std::begin (a4KR), std::end (a4KR), 0.0);
    std::fill (std::begin (a4LL), std::end (a4LL), 0.0); std::fill (std::begin (a4LR), std::end (a4LR), 0.0);
    std::fill (std::begin (a4ML), std::end (a4ML), 0.0); std::fill (std::begin (a4MR), std::end (a4MR), 0.0);
    std::fill (std::begin (a4NL), std::end (a4NL), 0.0); std::fill (std::begin (a4NR), std::end (a4NR), 0.0);
    std::fill (std::begin (a4OL), std::end (a4OL), 0.0); std::fill (std::begin (a4OR), std::end (a4OR), 0.0);
    std::fill (std::begin (a4PL), std::end (a4PL), 0.0); std::fill (std::begin (a4PR), std::end (a4PR), 0.0);

    c4AL = c4BL = c4CL = c4DL = c4EL = c4FL = c4GL = c4HL = 1;
    c4IL = c4JL = c4KL = c4LL = c4ML = c4NL = c4OL = c4PL = 1;
    c4AR = c4BR = c4CR = c4DR = c4ER = c4FR = c4GR = c4HR = 1;
    c4IR = c4JR = c4KR = c4LR = c4MR = c4NR = c4OR = c4PR = 1;

    f4AL = f4BL = f4CL = f4DL = 0.0;
    f4DR = f4HR = f4LR = f4PR = 0.0;

    std::fill (std::begin (b4AL), std::end (b4AL), 0.0); std::fill (std::begin (b4AR), std::end (b4AR), 0.0);
    std::fill (std::begin (b4BL), std::end (b4BL), 0.0); std::fill (std::begin (b4BR), std::end (b4BR), 0.0);
    std::fill (std::begin (b4CL), std::end (b4CL), 0.0); std::fill (std::begin (b4CR), std::end (b4CR), 0.0);
    std::fill (std::begin (b4DL), std::end (b4DL), 0.0); std::fill (std::begin (b4DR), std::end (b4DR), 0.0);
    std::fill (std::begin (b4EL), std::end (b4EL), 0.0); std::fill (std::begin (b4ER), std::end (b4ER), 0.0);
    std::fill (std::begin (b4FL), std::end (b4FL), 0.0); std::fill (std::begin (b4FR), std::end (b4FR), 0.0);
    std::fill (std::begin (b4GL), std::end (b4GL), 0.0); std::fill (std::begin (b4GR), std::end (b4GR), 0.0);
    std::fill (std::begin (b4HL), std::end (b4HL), 0.0); std::fill (std::begin (b4HR), std::end (b4HR), 0.0);
    std::fill (std::begin (b4IL), std::end (b4IL), 0.0); std::fill (std::begin (b4IR), std::end (b4IR), 0.0);
    std::fill (std::begin (b4JL), std::end (b4JL), 0.0); std::fill (std::begin (b4JR), std::end (b4JR), 0.0);
    std::fill (std::begin (b4KL), std::end (b4KL), 0.0); std::fill (std::begin (b4KR), std::end (b4KR), 0.0);
    std::fill (std::begin (b4LL), std::end (b4LL), 0.0); std::fill (std::begin (b4LR), std::end (b4LR), 0.0);
    std::fill (std::begin (b4ML), std::end (b4ML), 0.0); std::fill (std::begin (b4MR), std::end (b4MR), 0.0);
    std::fill (std::begin (b4NL), std::end (b4NL), 0.0); std::fill (std::begin (b4NR), std::end (b4NR), 0.0);
    std::fill (std::begin (b4OL), std::end (b4OL), 0.0); std::fill (std::begin (b4OR), std::end (b4OR), 0.0);
    std::fill (std::begin (b4PL), std::end (b4PL), 0.0); std::fill (std::begin (b4PR), std::end (b4PR), 0.0);

    e4AL = e4BL = e4CL = e4DL = e4EL = e4FL = e4GL = e4HL = 1;
    e4IL = e4JL = e4KL = e4LL = e4ML = e4NL = e4OL = e4PL = 1;
    e4AR = e4BR = e4CR = e4DR = e4ER = e4FR = e4GR = e4HR = 1;
    e4IR = e4JR = e4KR = e4LR = e4MR = e4NR = e4OR = e4PR = 1;

    g4AL = g4BL = g4CL = g4DL = 0.0;
    g4DR = g4HR = g4LR = g4PR = 0.0;

    std::fill (std::begin (bez), std::end (bez), 0.0);
    std::fill (std::begin (bezF), std::end (bezF), 0.0);
    bez[bez_cycle] = 1.0;
    bezF[bez_cycle] = 1.0;
}

void VerbTiny::processStereo (const float* in1, const float* in2,
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

    double reg4n = 0.03125 + ((1.0 - std::pow (1.0 - A, 2.0)) * 0.03125);
    double attenuate = 1.0 - (1.0 - std::pow (1.0 - A, 2.0));
    double derez = std::pow (B, 2.0);
    derez = std::fmin (std::fmax (derez / overallscale, 0.0001), 1.0);
    int bezFraction = (int) (1.0 / derez);
    double bezTrim = (double) bezFraction / (bezFraction + 1.0);
    derez = 1.0 / bezFraction;
    bezTrim = 1.0 - (derez * bezTrim);
    // die Revision verbindet die Bezier-Kurven genauer
    double derezFreq = std::pow (C, 2.0);
    derezFreq = std::fmin (std::fmax (derezFreq / overallscale, 0.0001), 1.0);
    int bezFreqFraction = (int) (1.0 / derezFreq);
    double bezFreqTrim = (double) bezFreqFraction / (bezFreqFraction + 1.0);
    derezFreq = 1.0 / bezFreqFraction;
    bezFreqTrim = 1.0 - (derezFreq * bezFreqTrim);
    // die Revision verbindet die Bezier-Kurven genauer
    double wider = D * 2.0;
    double wet = E;

    while (--sampleFrames >= 0)
    {
        double inputSampleL = *in1;
        double inputSampleR = *in2;
        if (std::fabs (inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
        if (std::fabs (inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;
        double drySampleL = inputSampleL;
        double drySampleR = inputSampleR;

        bez[bez_cycle] += derez;
        bez[bez_SampL] += (inputSampleL * attenuate * derez);
        bez[bez_SampR] += (inputSampleR * attenuate * derez);
        if (bez[bez_cycle] > 1.0) // Endpunkt erreicht -> ein Reverb-Sample
        {
            bez[bez_cycle] = 0.0;
            double mainSampleL = bez[bez_SampL];
            double dualmonoSampleL = bez[bez_SampR];
            // Workaround erhält das Cross-Matrix-System, aber beim initialen
            // Layering bekommt jede Seite beide Versionen — Blends fallen so
            // nie exakt zusammen.

            // linke Verbs
            a4AL[c4AL] = mainSampleL + (f4DR * reg4n);
            a4BL[c4BL] = mainSampleL + (f4HR * reg4n);
            a4CL[c4CL] = mainSampleL + (f4LR * reg4n);
            a4DL[c4DL] = mainSampleL + (f4PR * reg4n);
            b4AL[c4AL] = dualmonoSampleL + (g4AL * reg4n);
            b4BL[c4BL] = dualmonoSampleL + (g4BL * reg4n);
            b4CL[c4CL] = dualmonoSampleL + (g4CL * reg4n);
            b4DL[c4DL] = dualmonoSampleL + (g4DL * reg4n);

            c4AL++; if (c4AL < 0 || c4AL > d4A) c4AL = 0;
            c4BL++; if (c4BL < 0 || c4BL > d4B) c4BL = 0;
            c4CL++; if (c4CL < 0 || c4CL > d4C) c4CL = 0;
            c4DL++; if (c4DL < 0 || c4DL > d4D) c4DL = 0;

            double hA = a4AL[c4AL - ((c4AL > d4A) ? d4A + 1 : 0)];
            double hB = a4BL[c4BL - ((c4BL > d4B) ? d4B + 1 : 0)];
            double hC = a4CL[c4CL - ((c4CL > d4C) ? d4C + 1 : 0)];
            double hD = a4DL[c4DL - ((c4DL > d4D) ? d4D + 1 : 0)];
            a4EL[c4EL] = hA - (hB + hC + hD);
            a4FL[c4FL] = hB - (hA + hC + hD);
            a4GL[c4GL] = hC - (hA + hB + hD);
            a4HL[c4HL] = hD - (hA + hB + hC);
            hA = b4AL[c4AL - ((c4AL > d4A) ? d4A + 1 : 0)];
            hB = b4BL[c4BL - ((c4BL > d4B) ? d4B + 1 : 0)];
            hC = b4CL[c4CL - ((c4CL > d4C) ? d4C + 1 : 0)];
            hD = b4DL[c4DL - ((c4DL > d4D) ? d4D + 1 : 0)];
            b4EL[c4EL] = hA - (hB + hC + hD);
            b4FL[c4FL] = hB - (hA + hC + hD);
            b4GL[c4GL] = hC - (hA + hB + hD);
            b4HL[c4HL] = hD - (hA + hB + hC);

            c4EL++; if (c4EL < 0 || c4EL > d4E) c4EL = 0;
            c4FL++; if (c4FL < 0 || c4FL > d4F) c4FL = 0;
            c4GL++; if (c4GL < 0 || c4GL > d4G) c4GL = 0;
            c4HL++; if (c4HL < 0 || c4HL > d4H) c4HL = 0;

            hA = a4EL[c4EL - ((c4EL > d4E) ? d4E + 1 : 0)];
            hB = a4FL[c4FL - ((c4FL > d4F) ? d4F + 1 : 0)];
            hC = a4GL[c4GL - ((c4GL > d4G) ? d4G + 1 : 0)];
            hD = a4HL[c4HL - ((c4HL > d4H) ? d4H + 1 : 0)];
            a4IL[c4IL] = hA - (hB + hC + hD);
            a4JL[c4JL] = hB - (hA + hC + hD);
            a4KL[c4KL] = hC - (hA + hB + hD);
            a4LL[c4LL] = hD - (hA + hB + hC);
            hA = b4EL[c4EL - ((c4EL > d4E) ? d4E + 1 : 0)];
            hB = b4FL[c4FL - ((c4FL > d4F) ? d4F + 1 : 0)];
            hC = b4GL[c4GL - ((c4GL > d4G) ? d4G + 1 : 0)];
            hD = b4HL[c4HL - ((c4HL > d4H) ? d4H + 1 : 0)];
            b4IL[c4IL] = hA - (hB + hC + hD);
            b4JL[c4JL] = hB - (hA + hC + hD);
            b4KL[c4KL] = hC - (hA + hB + hD);
            b4LL[c4LL] = hD - (hA + hB + hC);

            c4IL++; if (c4IL < 0 || c4IL > d4I) c4IL = 0;
            c4JL++; if (c4JL < 0 || c4JL > d4J) c4JL = 0;
            c4KL++; if (c4KL < 0 || c4KL > d4K) c4KL = 0;
            c4LL++; if (c4LL < 0 || c4LL > d4L) c4LL = 0;

            hA = a4IL[c4IL - ((c4IL > d4I) ? d4I + 1 : 0)];
            hB = a4JL[c4JL - ((c4JL > d4J) ? d4J + 1 : 0)];
            hC = a4KL[c4KL - ((c4KL > d4K) ? d4K + 1 : 0)];
            hD = a4LL[c4LL - ((c4LL > d4L) ? d4L + 1 : 0)];
            a4ML[c4ML] = hA - (hB + hC + hD);
            a4NL[c4NL] = hB - (hA + hC + hD);
            a4OL[c4OL] = hC - (hA + hB + hD);
            a4PL[c4PL] = hD - (hA + hB + hC);
            hA = b4IL[c4IL - ((c4IL > d4I) ? d4I + 1 : 0)];
            hB = b4JL[c4JL - ((c4JL > d4J) ? d4J + 1 : 0)];
            hC = b4KL[c4KL - ((c4KL > d4K) ? d4K + 1 : 0)];
            hD = b4LL[c4LL - ((c4LL > d4L) ? d4L + 1 : 0)];
            b4ML[c4ML] = hA - (hB + hC + hD);
            b4NL[c4NL] = hB - (hA + hC + hD);
            b4OL[c4OL] = hC - (hA + hB + hD);
            b4PL[c4PL] = hD - (hA + hB + hC);

            c4ML++; if (c4ML < 0 || c4ML > d4M) c4ML = 0;
            c4NL++; if (c4NL < 0 || c4NL > d4N) c4NL = 0;
            c4OL++; if (c4OL < 0 || c4OL > d4O) c4OL = 0;
            c4PL++; if (c4PL < 0 || c4PL > d4P) c4PL = 0;

            hA = a4ML[c4ML - ((c4ML > d4M) ? d4M + 1 : 0)];
            hB = a4NL[c4NL - ((c4NL > d4N) ? d4N + 1 : 0)];
            hC = a4OL[c4OL - ((c4OL > d4O) ? d4O + 1 : 0)];
            hD = a4PL[c4PL - ((c4PL > d4P) ? d4P + 1 : 0)];
            f4AL = hA - (hB + hC + hD);
            f4BL = hB - (hA + hC + hD);
            f4CL = hC - (hA + hB + hD);
            f4DL = hD - (hA + hB + hC); // noch nicht cross-channel
            mainSampleL = (hA + hB + hC + hD) * 0.125;

            hA = b4ML[c4ML - ((c4ML > d4M) ? d4M + 1 : 0)];
            hB = b4NL[c4NL - ((c4NL > d4N) ? d4N + 1 : 0)];
            hC = b4OL[c4OL - ((c4OL > d4O) ? d4O + 1 : 0)];
            hD = b4PL[c4PL - ((c4PL > d4P) ? d4P + 1 : 0)];
            g4AL = hA - (hB + hC + hD);
            g4BL = hB - (hA + hC + hD);
            g4CL = hC - (hA + hB + hD);
            g4DL = hD - (hA + hB + hC);
            dualmonoSampleL = (hA + hB + hC + hD) * 0.125;

            double mainSampleR = bez[bez_SampR]; // beginne primären Reverb
            double dualmonoSampleR = bez[bez_SampL];

            // rechte Verbs
            a4DR[c4DR] = mainSampleR + (f4AL * reg4n);
            a4HR[c4HR] = mainSampleR + (f4BL * reg4n);
            a4LR[c4LR] = mainSampleR + (f4CL * reg4n);
            a4PR[c4PR] = mainSampleR + (f4DL * reg4n);
            b4DR[c4DR] = dualmonoSampleR + (g4DR * reg4n);
            b4HR[c4HR] = dualmonoSampleR + (g4HR * reg4n);
            b4LR[c4LR] = dualmonoSampleR + (g4LR * reg4n);
            b4PR[c4PR] = dualmonoSampleR + (g4PR * reg4n);

            c4DR++; if (c4DR < 0 || c4DR > d4D) c4DR = 0;
            c4HR++; if (c4HR < 0 || c4HR > d4H) c4HR = 0;
            c4LR++; if (c4LR < 0 || c4LR > d4L) c4LR = 0;
            c4PR++; if (c4PR < 0 || c4PR > d4P) c4PR = 0;

            hA = a4DR[c4DR - ((c4DR > d4D) ? d4D + 1 : 0)];
            hB = a4HR[c4HR - ((c4HR > d4H) ? d4H + 1 : 0)];
            hC = a4LR[c4LR - ((c4LR > d4L) ? d4L + 1 : 0)];
            hD = a4PR[c4PR - ((c4PR > d4P) ? d4P + 1 : 0)];
            a4CR[c4CR] = hA - (hB + hC + hD);
            a4GR[c4GR] = hB - (hA + hC + hD);
            a4KR[c4KR] = hC - (hA + hB + hD);
            a4OR[c4OR] = hD - (hA + hB + hC);
            hA = b4DR[c4DR - ((c4DR > d4D) ? d4D + 1 : 0)];
            hB = b4HR[c4HR - ((c4HR > d4H) ? d4H + 1 : 0)];
            hC = b4LR[c4LR - ((c4LR > d4L) ? d4L + 1 : 0)];
            hD = b4PR[c4PR - ((c4PR > d4P) ? d4P + 1 : 0)];
            b4CR[c4CR] = hA - (hB + hC + hD);
            b4GR[c4GR] = hB - (hA + hC + hD);
            b4KR[c4KR] = hC - (hA + hB + hD);
            b4OR[c4OR] = hD - (hA + hB + hC);

            c4CR++; if (c4CR < 0 || c4CR > d4C) c4CR = 0;
            c4GR++; if (c4GR < 0 || c4GR > d4G) c4GR = 0;
            c4KR++; if (c4KR < 0 || c4KR > d4K) c4KR = 0;
            c4OR++; if (c4OR < 0 || c4OR > d4O) c4OR = 0;

            hA = a4CR[c4CR - ((c4CR > d4C) ? d4C + 1 : 0)];
            hB = a4GR[c4GR - ((c4GR > d4G) ? d4G + 1 : 0)];
            hC = a4KR[c4KR - ((c4KR > d4K) ? d4K + 1 : 0)];
            hD = a4OR[c4OR - ((c4OR > d4O) ? d4O + 1 : 0)];
            a4BR[c4BR] = hA - (hB + hC + hD);
            a4FR[c4FR] = hB - (hA + hC + hD);
            a4JR[c4JR] = hC - (hA + hB + hD);
            a4NR[c4NR] = hD - (hA + hB + hC);
            hA = b4CR[c4CR - ((c4CR > d4C) ? d4C + 1 : 0)];
            hB = b4GR[c4GR - ((c4GR > d4G) ? d4G + 1 : 0)];
            hC = b4KR[c4KR - ((c4KR > d4K) ? d4K + 1 : 0)];
            hD = b4OR[c4OR - ((c4OR > d4O) ? d4O + 1 : 0)];
            b4BR[c4BR] = hA - (hB + hC + hD);
            b4FR[c4FR] = hB - (hA + hC + hD);
            b4JR[c4JR] = hC - (hA + hB + hD);
            b4NR[c4NR] = hD - (hA + hB + hC);

            c4BR++; if (c4BR < 0 || c4BR > d4B) c4BR = 0;
            c4FR++; if (c4FR < 0 || c4FR > d4F) c4FR = 0;
            c4JR++; if (c4JR < 0 || c4JR > d4J) c4JR = 0;
            c4NR++; if (c4NR < 0 || c4NR > d4N) c4NR = 0;

            hA = a4BR[c4BR - ((c4BR > d4B) ? d4B + 1 : 0)];
            hB = a4FR[c4FR - ((c4FR > d4F) ? d4F + 1 : 0)];
            hC = a4JR[c4JR - ((c4JR > d4J) ? d4J + 1 : 0)];
            hD = a4NR[c4NR - ((c4NR > d4N) ? d4N + 1 : 0)];
            a4AR[c4AR] = hA - (hB + hC + hD);
            a4ER[c4ER] = hB - (hA + hC + hD);
            a4IR[c4IR] = hC - (hA + hB + hD);
            a4MR[c4MR] = hD - (hA + hB + hC);
            hA = b4BR[c4BR - ((c4BR > d4B) ? d4B + 1 : 0)];
            hB = b4FR[c4FR - ((c4FR > d4F) ? d4F + 1 : 0)];
            hC = b4JR[c4JR - ((c4JR > d4J) ? d4J + 1 : 0)];
            hD = b4NR[c4NR - ((c4NR > d4N) ? d4N + 1 : 0)];
            b4AR[c4AR] = hA - (hB + hC + hD);
            b4ER[c4ER] = hB - (hA + hC + hD);
            b4IR[c4IR] = hC - (hA + hB + hD);
            b4MR[c4MR] = hD - (hA + hB + hC);

            c4AR++; if (c4AR < 0 || c4AR > d4A) c4AR = 0;
            c4ER++; if (c4ER < 0 || c4ER > d4E) c4ER = 0;
            c4IR++; if (c4IR < 0 || c4IR > d4I) c4IR = 0;
            c4MR++; if (c4MR < 0 || c4MR > d4M) c4MR = 0;

            hA = a4AR[c4AR - ((c4AR > d4A) ? d4A + 1 : 0)];
            hB = a4ER[c4ER - ((c4ER > d4E) ? d4E + 1 : 0)];
            hC = a4IR[c4IR - ((c4IR > d4I) ? d4I + 1 : 0)];
            hD = a4MR[c4MR - ((c4MR > d4M) ? d4M + 1 : 0)];
            f4DR = hA - (hB + hC + hD);
            f4HR = hB - (hA + hC + hD);
            f4LR = hC - (hA + hB + hD);
            f4PR = hD - (hA + hB + hC);
            mainSampleR = (hA + hB + hC + hD) * 0.125;

            hA = b4AR[c4AR - ((c4AR > d4A) ? d4A + 1 : 0)];
            hB = b4ER[c4ER - ((c4ER > d4E) ? d4E + 1 : 0)];
            hC = b4IR[c4IR - ((c4IR > d4I) ? d4I + 1 : 0)];
            hD = b4MR[c4MR - ((c4MR > d4M) ? d4M + 1 : 0)];
            g4DR = hA - (hB + hC + hD);
            g4HR = hB - (hA + hC + hD);
            g4LR = hC - (hA + hB + hD);
            g4PR = hD - (hA + hB + hC);
            dualmonoSampleR = (hA + hB + hC + hD) * 0.125;
            // dualmono ist wider = 1.0 in der Mitte, mit mainsample 0.0 und
            // 2.0 (nur an den Rändern); mainsample gegenphasig über 1.0.
            // Statt die Arrays neu aufzubauen, bleibt die Cross-Matrix
            // exakt gleich — die Seiten für den initialen Reverb werden
            // vertauscht. dualmono bleibt dadurch komplett dualmono und
            // wird für die Breite nur etwas beigemischt.

            if (wider < 1.0)
            {
                inputSampleL = (dualmonoSampleR * wider) + (mainSampleL * (1.0 - wider));
                inputSampleR = (dualmonoSampleL * wider) + (mainSampleR * (1.0 - wider));
            }
            else
            {
                inputSampleL = (dualmonoSampleR * (2.0 - wider)) + (mainSampleL * (wider - 1.0));
                inputSampleR = (dualmonoSampleL * (2.0 - wider)) + (-mainSampleR * (wider - 1.0));
            }

            bez[bez_CL] = bez[bez_BL];
            bez[bez_BL] = bez[bez_AL];
            bez[bez_AL] = inputSampleL;
            bez[bez_SampL] = 0.0;

            bez[bez_CR] = bez[bez_BR];
            bez[bez_BR] = bez[bez_AR];
            bez[bez_AR] = inputSampleR;
            bez[bez_SampR] = 0.0;
        }
        double X = bez[bez_cycle] * bezTrim;
        double CBL = (bez[bez_CL] * (1.0 - X)) + (bez[bez_BL] * X);
        double CBR = (bez[bez_CR] * (1.0 - X)) + (bez[bez_BR] * X);
        double BAL = (bez[bez_BL] * (1.0 - X)) + (bez[bez_AL] * X);
        double BAR = (bez[bez_BR] * (1.0 - X)) + (bez[bez_AR] * X);
        inputSampleL = (bez[bez_BL] + (CBL * (1.0 - X)) + (BAL * X)) * -0.25;
        inputSampleR = (bez[bez_BR] + (CBR * (1.0 - X)) + (BAR * X)) * -0.25;

        bezF[bez_cycle] += derezFreq;
        bezF[bez_SampL] += (inputSampleL * derezFreq);
        bezF[bez_SampR] += (inputSampleR * derezFreq);
        if (bezF[bez_cycle] > 1.0) // Endpunkt erreicht -> ein Filter-Sample
        {
            bezF[bez_cycle] = 0.0;
            bezF[bez_CL] = bezF[bez_BL];
            bezF[bez_BL] = bezF[bez_AL];
            bezF[bez_AL] = bezF[bez_SampL];
            bezF[bez_SampL] = 0.0;
            bezF[bez_CR] = bezF[bez_BR];
            bezF[bez_BR] = bezF[bez_AR];
            bezF[bez_AR] = bezF[bez_SampR];
            bezF[bez_SampR] = 0.0;
        }
        X = bezF[bez_cycle] * bezFreqTrim;
        double CBLfreq = (bezF[bez_CL] * (1.0 - X)) + (bezF[bez_BL] * X);
        double BALfreq = (bezF[bez_BL] * (1.0 - X)) + (bezF[bez_AL] * X);
        inputSampleL = (bezF[bez_BL] + (CBLfreq * (1.0 - X)) + (BALfreq * X)) * 0.5;
        double CBRfreq = (bezF[bez_CR] * (1.0 - X)) + (bezF[bez_BR] * X);
        double BARfreq = (bezF[bez_BR] * (1.0 - X)) + (bezF[bez_AR] * X);
        inputSampleR = (bezF[bez_BR] + (CBRfreq * (1.0 - X)) + (BARfreq * X)) * 0.5;
        // Reverb wird separat gefiltert, nachdem er erzeugt wurde

        inputSampleL = (inputSampleL * wet) + (drySampleL * (1.0 - wet));
        inputSampleR = (inputSampleR * wet) + (drySampleR * (1.0 - wet));

        // 32-Bit-Stereo-Float-Dither
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

        *out1 = (float) inputSampleL;
        *out2 = (float) inputSampleR;

        ++in1;
        ++in2;
        ++out1;
        ++out2;
    }
}

} // namespace conduit::airwindows
