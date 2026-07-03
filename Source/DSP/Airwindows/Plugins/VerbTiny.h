/* ========================================
 *  VerbTiny - VerbTiny.h
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus plugins/LinuxVST/src/VerbTiny (github.com/airwindows/airwindows) —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Alle Delay-Puffer sind fest große Klassenmember (kein new/malloc im
 *  Verarbeitungspfad, CLAUDE.md 3.1) — exakt wie im Original.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** VerbTiny — kompakter Vier-mal-vier-Matrix-Reverb (436-Seat-Theater-Preset)
    mit Bezier-Undersampling für Hall- und Filterpfad getrennt (Replace/
    Derez/Filter/Wider/Dry-Wet). */
class VerbTiny final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Replace
        kParamB = 1, // Derez
        kParamC = 2, // Filter
        kParamD = 3, // Wider
        kParamE = 4, // Dry/Wet
        kNumParameters = 5
    };

    VerbTiny() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    // Delay-Größen des Originals (436seat1365253x4) — Namen/Werte 1:1 aus
    // VerbTiny.h übernommen (d4A..d4P).
    static constexpr int d4A = 136;
    static constexpr int d4B = 52;
    static constexpr int d4C = 53;
    static constexpr int d4D = 1261;
    static constexpr int d4E = 209;
    static constexpr int d4F = 473;
    static constexpr int d4G = 549;
    static constexpr int d4H = 29;
    static constexpr int d4I = 92;
    static constexpr int d4J = 1137;
    static constexpr int d4K = 1406;
    static constexpr int d4L = 994;
    static constexpr int d4M = 1314;
    static constexpr int d4N = 191;
    static constexpr int d4O = 1263;
    static constexpr int d4P = 103;

    // "a4"-Matrix: Haupt-/Cross-Kanal-Reverb (mainSample-Pfad)
    double a4AL[d4A + 5] {}, a4AR[d4A + 5] {};
    double a4BL[d4B + 5] {}, a4BR[d4B + 5] {};
    double a4CL[d4C + 5] {}, a4CR[d4C + 5] {};
    double a4DL[d4D + 5] {}, a4DR[d4D + 5] {};
    double a4EL[d4E + 5] {}, a4ER[d4E + 5] {};
    double a4FL[d4F + 5] {}, a4FR[d4F + 5] {};
    double a4GL[d4G + 5] {}, a4GR[d4G + 5] {};
    double a4HL[d4H + 5] {}, a4HR[d4H + 5] {};
    double a4IL[d4I + 5] {}, a4IR[d4I + 5] {};
    double a4JL[d4J + 5] {}, a4JR[d4J + 5] {};
    double a4KL[d4K + 5] {}, a4KR[d4K + 5] {};
    double a4LL[d4L + 5] {}, a4LR[d4L + 5] {};
    double a4ML[d4M + 5] {}, a4MR[d4M + 5] {};
    double a4NL[d4N + 5] {}, a4NR[d4N + 5] {};
    double a4OL[d4O + 5] {}, a4OR[d4O + 5] {};
    double a4PL[d4P + 5] {}, a4PR[d4P + 5] {};

    int c4AL = 1, c4BL = 1, c4CL = 1, c4DL = 1, c4EL = 1, c4FL = 1, c4GL = 1, c4HL = 1;
    int c4IL = 1, c4JL = 1, c4KL = 1, c4LL = 1, c4ML = 1, c4NL = 1, c4OL = 1, c4PL = 1;
    int c4AR = 1, c4BR = 1, c4CR = 1, c4DR = 1, c4ER = 1, c4FR = 1, c4GR = 1, c4HR = 1;
    int c4IR = 1, c4JR = 1, c4KR = 1, c4LR = 1, c4MR = 1, c4NR = 1, c4OR = 1, c4PR = 1;

    // Feedback aus dem letzten Reverb-Sample zurück in den "a4"-Eingang
    // (nur die acht tatsächlich rückgekoppelten Ausgänge)
    double f4AL = 0.0, f4BL = 0.0, f4CL = 0.0, f4DL = 0.0;
    double f4DR = 0.0, f4HR = 0.0, f4LR = 0.0, f4PR = 0.0;

    // "b4"-Matrix: dualmono/Wideness-Pfad, eigene Feedback-Variablen (g4*)
    double b4AL[d4A + 5] {}, b4AR[d4A + 5] {};
    double b4BL[d4B + 5] {}, b4BR[d4B + 5] {};
    double b4CL[d4C + 5] {}, b4CR[d4C + 5] {};
    double b4DL[d4D + 5] {}, b4DR[d4D + 5] {};
    double b4EL[d4E + 5] {}, b4ER[d4E + 5] {};
    double b4FL[d4F + 5] {}, b4FR[d4F + 5] {};
    double b4GL[d4G + 5] {}, b4GR[d4G + 5] {};
    double b4HL[d4H + 5] {}, b4HR[d4H + 5] {};
    double b4IL[d4I + 5] {}, b4IR[d4I + 5] {};
    double b4JL[d4J + 5] {}, b4JR[d4J + 5] {};
    double b4KL[d4K + 5] {}, b4KR[d4K + 5] {};
    double b4LL[d4L + 5] {}, b4LR[d4L + 5] {};
    double b4ML[d4M + 5] {}, b4MR[d4M + 5] {};
    double b4NL[d4N + 5] {}, b4NR[d4N + 5] {};
    double b4OL[d4O + 5] {}, b4OR[d4O + 5] {};
    double b4PL[d4P + 5] {}, b4PR[d4P + 5] {};

    int e4AL = 1, e4BL = 1, e4CL = 1, e4DL = 1, e4EL = 1, e4FL = 1, e4GL = 1, e4HL = 1;
    int e4IL = 1, e4JL = 1, e4KL = 1, e4LL = 1, e4ML = 1, e4NL = 1, e4OL = 1, e4PL = 1;
    int e4AR = 1, e4BR = 1, e4CR = 1, e4DR = 1, e4ER = 1, e4FR = 1, e4GR = 1, e4HR = 1;
    int e4IR = 1, e4JR = 1, e4KR = 1, e4LR = 1, e4MR = 1, e4NR = 1, e4OR = 1, e4PR = 1;

    double g4AL = 0.0, g4BL = 0.0, g4CL = 0.0, g4DL = 0.0;
    double g4DR = 0.0, g4HR = 0.0, g4LR = 0.0, g4PR = 0.0;

    // Bezier-Undersampling: getrennter Zustand für Hall-Erzeugung (bez)
    // und Nachfilterung (bezF) — Original-Enum-Reihenfolge übernommen.
    enum
    {
        bez_AL,
        bez_AR,
        bez_BL,
        bez_BR,
        bez_CL,
        bez_CR,
        bez_SampL,
        bez_SampR,
        bez_cycle,
        bez_total
    };
    double bez[bez_total] {};
    double bezF[bez_total] {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VerbTiny)
};

} // namespace conduit::airwindows
