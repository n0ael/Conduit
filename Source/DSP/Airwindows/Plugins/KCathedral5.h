/* ========================================
 *  kCathedral5 - KCathedral5.h
 *  Copyright (c) 2011 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus plugins/LinuxVST/src/kCathedral5 (github.com/airwindows/airwindows) —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Alle Delay-Puffer sind fest große Klassenmember (kein new/malloc im
 *  Verarbeitungspfad, CLAUDE.md 3.1) — exakt wie im Original.
 *
 *  Struktur: "3x3"-Householder-Kaskade (a3*, Early-Reflections mit
 *  positionsabhängigen Delay-Längen aus dem `early[]`-Array) speist eine
 *  zweite Bezier-Undersampling-Stufe (bezF[], Filter-Parameter) vor der
 *  "6x6"-Householder-Kaskade (a6*); äußeres Bezier-Undersampling (bez[])
 *  für den DeRez-Parameter, jeweils mit Running-Average-Rekonstruktion
 *  (bez_AvgInSamp und bez_AvgOutSamp Indizes, Revised-Algorithmus ggü. kBeyond).
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** kCathedral5 — Householder-FDN-Reverb mit positionsabhängigen frühen
    Reflexionen (Positin-Parameter) und doppeltem Bezier-Undersampling
    (Derez + Filter) (Regen/Derez/Filter/EarlyRF/Positin/Dry-Wet). */
class KCathedral5 final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Regen
        kParamB = 1, // Derez
        kParamC = 2, // Filter
        kParamD = 3, // EarlyRF
        kParamE = 4, // Positin
        kParamF = 5, // Dry/Wet
        kNumParameters = 6
    };

    KCathedral5() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    // Original-Konstanten (6 to 259 ms, 2094 seat arena — "3x3"-Kaskade,
    // Basisgrößen der Arrays; die tatsächlichen Lauflängen pro Kanal werden
    // pro Block aus dem early[]-Array via Positin-Parameter gewählt, daher
    // hier als Array-Obergrenze verwendet)
    static constexpr int d3A = 1683; static constexpr int d3B = 2395; static constexpr int d3C = 2432;
    static constexpr int d3D = 1552; static constexpr int d3E = 1735; static constexpr int d3F = 2392;
    static constexpr int d3G = 1364; static constexpr int d3H = 1468; static constexpr int d3I = 1961;

    // Original-Konstanten (6 to 259 ms, 2094 seat arena — "6x6"-Kaskade)
    static constexpr int d6A = 2392; static constexpr int d6B = 710;  static constexpr int d6C = 35;
    static constexpr int d6D = 396;  static constexpr int d6E = 2395; static constexpr int d6F = 81;
    static constexpr int d6G = 20;   static constexpr int d6H = 2432; static constexpr int d6I = 1031;
    static constexpr int d6J = 529;  static constexpr int d6K = 921;  static constexpr int d6L = 116;
    static constexpr int d6M = 1961; static constexpr int d6N = 747;  static constexpr int d6O = 160;
    static constexpr int d6P = 231;  static constexpr int d6Q = 820;  static constexpr int d6R = 493;
    static constexpr int d6S = 188;  static constexpr int d6T = 412;  static constexpr int d6U = 1364;
    static constexpr int d6V = 43;   static constexpr int d6W = 206;  static constexpr int d6X = 855;
    static constexpr int d6Y = 216;  static constexpr int d6ZA = 1735; static constexpr int d6ZB = 53;
    static constexpr int d6ZC = 1468; static constexpr int d6ZD = 1272; static constexpr int d6ZE = 1683;
    static constexpr int d6ZF = 1287; static constexpr int d6ZG = 8;   static constexpr int d6ZH = 14;
    static constexpr int d6ZI = 1552; static constexpr int d6ZJ = 88;  static constexpr int d6ZK = 914;

    // Positionsabhängige Early-Reflection-Delaylängen (27 Positionen, 9
    // Werte je Position aus dem sortierten Original-Array `early[]` gewählt
    // — 1:1 aus dem Original übernommen, dient als Laufzeit-Lookup für
    // ld3A..ld3I in processStereo(), NICHT als Array-Größe (die bleibt d3*)).
    static constexpr int kEarly[] = {
        8, 14, 20, 35, 43, 53, 81, 88, 116, 160, 188, 206, 216, 231, 396, 412,
        493, 529, 710, 747, 820, 855, 914, 921, 1031, 1272, 1287, 1364, 1468,
        1552, 1683, 1735, 1961, 2392, 2395, 2432
    };

    double a3AL[d3A + 5] {}, a3BL[d3B + 5] {}, a3CL[d3C + 5] {}, a3DL[d3D + 5] {}, a3EL[d3E + 5] {};
    double a3FL[d3F + 5] {}, a3GL[d3G + 5] {}, a3HL[d3H + 5] {}, a3IL[d3I + 5] {};
    double a3AR[d3A + 5] {}, a3BR[d3B + 5] {}, a3CR[d3C + 5] {}, a3DR[d3D + 5] {}, a3ER[d3E + 5] {};
    double a3FR[d3F + 5] {}, a3GR[d3G + 5] {}, a3HR[d3H + 5] {}, a3IR[d3I + 5] {};
    int c3AL = 1, c3AR = 1, c3BL = 1, c3BR = 1, c3CL = 1, c3CR = 1, c3DL = 1, c3DR = 1, c3EL = 1, c3ER = 1;
    int c3FL = 1, c3FR = 1, c3GL = 1, c3GR = 1, c3HL = 1, c3HR = 1, c3IL = 1, c3IR = 1;

    double a6AL[d6A + 5] {}, a6BL[d6B + 5] {}, a6CL[d6C + 5] {}, a6DL[d6D + 5] {}, a6EL[d6E + 5] {};
    double a6FL[d6F + 5] {}, a6GL[d6G + 5] {}, a6HL[d6H + 5] {}, a6IL[d6I + 5] {}, a6JL[d6J + 5] {};
    double a6KL[d6K + 5] {}, a6LL[d6L + 5] {}, a6ML[d6M + 5] {}, a6NL[d6N + 5] {}, a6OL[d6O + 5] {};
    double a6PL[d6P + 5] {}, a6QL[d6Q + 5] {}, a6RL[d6R + 5] {}, a6SL[d6S + 5] {}, a6TL[d6T + 5] {};
    double a6UL[d6U + 5] {}, a6VL[d6V + 5] {}, a6WL[d6W + 5] {}, a6XL[d6X + 5] {}, a6YL[d6Y + 5] {};
    double a6ZAL[d6ZA + 5] {}, a6ZBL[d6ZB + 5] {}, a6ZCL[d6ZC + 5] {}, a6ZDL[d6ZD + 5] {}, a6ZEL[d6ZE + 5] {};
    double a6ZFL[d6ZF + 5] {}, a6ZGL[d6ZG + 5] {}, a6ZHL[d6ZH + 5] {}, a6ZIL[d6ZI + 5] {}, a6ZJL[d6ZJ + 5] {};
    double a6ZKL[d6ZK + 5] {};
    double a6AR[d6A + 5] {}, a6BR[d6B + 5] {}, a6CR[d6C + 5] {}, a6DR[d6D + 5] {}, a6ER[d6E + 5] {};
    double a6FR[d6F + 5] {}, a6GR[d6G + 5] {}, a6HR[d6H + 5] {}, a6IR[d6I + 5] {}, a6JR[d6J + 5] {};
    double a6KR[d6K + 5] {}, a6LR[d6L + 5] {}, a6MR[d6M + 5] {}, a6NR[d6N + 5] {}, a6OR[d6O + 5] {};
    double a6PR[d6P + 5] {}, a6QR[d6Q + 5] {}, a6RR[d6R + 5] {}, a6SR[d6S + 5] {}, a6TR[d6T + 5] {};
    double a6UR[d6U + 5] {}, a6VR[d6V + 5] {}, a6WR[d6W + 5] {}, a6XR[d6X + 5] {}, a6YR[d6Y + 5] {};
    double a6ZAR[d6ZA + 5] {}, a6ZBR[d6ZB + 5] {}, a6ZCR[d6ZC + 5] {}, a6ZDR[d6ZD + 5] {}, a6ZER[d6ZE + 5] {};
    double a6ZFR[d6ZF + 5] {}, a6ZGR[d6ZG + 5] {}, a6ZHR[d6ZH + 5] {}, a6ZIR[d6ZI + 5] {}, a6ZJR[d6ZJ + 5] {};
    double a6ZKR[d6ZK + 5] {};

    int c6AL = 1, c6BL = 1, c6CL = 1, c6DL = 1, c6EL = 1, c6FL = 1, c6GL = 1, c6HL = 1, c6IL = 1;
    int c6JL = 1, c6KL = 1, c6LL = 1, c6ML = 1, c6NL = 1, c6OL = 1, c6PL = 1, c6QL = 1, c6RL = 1;
    int c6SL = 1, c6TL = 1, c6UL = 1, c6VL = 1, c6WL = 1, c6XL = 1, c6YL = 1, c6ZAL = 1, c6ZBL = 1;
    int c6ZCL = 1, c6ZDL = 1, c6ZEL = 1, c6ZFL = 1, c6ZGL = 1, c6ZHL = 1, c6ZIL = 1, c6ZJL = 1, c6ZKL = 1;
    int c6AR = 1, c6BR = 1, c6CR = 1, c6DR = 1, c6ER = 1, c6FR = 1, c6GR = 1, c6HR = 1, c6IR = 1;
    int c6JR = 1, c6KR = 1, c6LR = 1, c6MR = 1, c6NR = 1, c6OR = 1, c6PR = 1, c6QR = 1, c6RR = 1;
    int c6SR = 1, c6TR = 1, c6UR = 1, c6VR = 1, c6WR = 1, c6XR = 1, c6YR = 1, c6ZAR = 1, c6ZBR = 1;
    int c6ZCR = 1, c6ZDR = 1, c6ZER = 1, c6ZFR = 1, c6ZGR = 1, c6ZHR = 1, c6ZIR = 1, c6ZJR = 1, c6ZKR = 1;

    double f6AL = 0.0, f6BL = 0.0, f6CL = 0.0, f6DL = 0.0, f6EL = 0.0, f6FL = 0.0;
    double f6FR = 0.0, f6LR = 0.0, f6RR = 0.0, f6XR = 0.0, f6ZER = 0.0, f6ZKR = 0.0;
    double avg6L = 0.0, avg6R = 0.0;

    enum
    {
        bez_AL, bez_AR, bez_BL, bez_BR, bez_CL, bez_CR,
        bez_InL, bez_InR, bez_UnInL, bez_UnInR, bez_SampL, bez_SampR,
        bez_AvgInSampL, bez_AvgInSampR, bez_AvgOutSampL, bez_AvgOutSampR,
        bez_cycle,
        bez_total
    }; // die Bezier-Kurven-Rekonstruktion fürs Undersampling (Derez-Stufe)
    double bez[bez_total] {};

    // zweite, unabhängige Bezier-Undersampling-Stufe für den Filter-Parameter
    double bezF[bez_total] {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KCathedral5)
};

} // namespace conduit::airwindows
