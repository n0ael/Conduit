/* ========================================
 *  kWoodRoom - kWoodRoom.h
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus plugins/LinuxVST/src/kWoodRoom (github.com/airwindows/airwindows) —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 *  Klassenname KWoodRoom (C++-Konvention, großgeschrieben) — Original-
 *  Anzeigename "kWoodRoom" bleibt in getEffectName() erhalten.
 *  Alle Delay-Puffer sind fest große Klassenmember (kein new/malloc im
 *  Verarbeitungspfad, CLAUDE.md 3.1) — exakt wie im Original.
 *  Original deklariert zusätzlich avg6L/avg6R als Member — im gesamten
 *  Proc-Körper (processReplacing UND processDoubleReplacing) nie gelesen,
 *  nur im Konstruktor auf 0.0 gesetzt (totes Altbestand-Feld), hier bewusst
 *  weggelassen (sonst -Wunused-private-field unter Clang/-Werror).
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** kWoodRoom — Drei-mal-drei-Early-Reflections + Sechs-mal-sechs-Matrix-
    Reverb (249-Seat-Club-Preset) mit wählbarer Early-Reflection-Position
    (Regen/Derez/Filter/EarlyRF/Positin/Dry-Wet). */
class KWoodRoom final : public AirwindowsPlugin
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

    KWoodRoom() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    // Early-Reflection-Delay-Größen (249seat154832109x6) — d3A..d3I
    static constexpr int d3A = 581;
    static constexpr int d3B = 831;
    static constexpr int d3C = 832;
    static constexpr int d3D = 574;
    static constexpr int d3E = 598;
    static constexpr int d3F = 685;
    static constexpr int d3G = 499;
    static constexpr int d3H = 573;
    static constexpr int d3I = 655;

    // Haupt-Reverb-Delay-Größen — d6A..d6ZK (Original-Bezeichner erhalten,
    // "Z"-Präfix ist Teil des Airwindows-Namensschemas nach Erschöpfen A..Y)
    static constexpr int d6A = 154;
    static constexpr int d6B = 832;
    static constexpr int d6C = 109;
    static constexpr int d6D = 685;
    static constexpr int d6E = 33;
    static constexpr int d6F = 12;
    static constexpr int d6G = 27;
    static constexpr int d6H = 30;
    static constexpr int d6I = 339;
    static constexpr int d6J = 499;
    static constexpr int d6K = 296;
    static constexpr int d6L = 169;
    static constexpr int d6M = 169;
    static constexpr int d6N = 831;
    static constexpr int d6O = 15;
    static constexpr int d6P = 411;
    static constexpr int d6Q = 238;
    static constexpr int d6R = 68;
    static constexpr int d6S = 0;
    static constexpr int d6T = 8;
    static constexpr int d6U = 655;
    static constexpr int d6V = 581;
    static constexpr int d6W = 465;
    static constexpr int d6X = 173;
    static constexpr int d6Y = 3;
    static constexpr int d6ZA = 96;
    static constexpr int d6ZB = 573;
    static constexpr int d6ZC = 243;
    static constexpr int d6ZD = 30;
    static constexpr int d6ZE = 188;
    static constexpr int d6ZF = 291;
    static constexpr int d6ZG = 11;
    static constexpr int d6ZH = 372;
    static constexpr int d6ZI = 574;
    static constexpr int d6ZJ = 100;
    static constexpr int d6ZK = 598;

    // Positions-Tabelle für die Early-Reflections — Original-Werte 1:1,
    // Index (E * 27) + Offset 0..8 wählt 9 aufeinanderfolgende Werte.
    static constexpr int early[] = {
        0, 3, 8, 11, 12, 15, 27, 30, 30, 33, 68, 96, 100, 109, 154, 169, 169,
        173, 188, 238, 243, 291, 296, 339, 372, 411, 465, 499, 573, 574, 581,
        598, 655, 685, 831, 832
    };

    // "a3"-Matrix: Early Reflections (3x3-Kreuzmatrix, dual Kanal-Routing)
    double a3AL[d3A + 5] {}, a3AR[d3A + 5] {};
    double a3BL[d3B + 5] {}, a3BR[d3B + 5] {};
    double a3CL[d3C + 5] {}, a3CR[d3C + 5] {};
    double a3DL[d3D + 5] {}, a3DR[d3D + 5] {};
    double a3EL[d3E + 5] {}, a3ER[d3E + 5] {};
    double a3FL[d3F + 5] {}, a3FR[d3F + 5] {};
    double a3GL[d3G + 5] {}, a3GR[d3G + 5] {};
    double a3HL[d3H + 5] {}, a3HR[d3H + 5] {};
    double a3IL[d3I + 5] {}, a3IR[d3I + 5] {};
    int c3AL = 1, c3AR = 1, c3BL = 1, c3BR = 1, c3CL = 1, c3CR = 1, c3DL = 1, c3DR = 1, c3EL = 1, c3ER = 1;
    int c3FL = 1, c3FR = 1, c3GL = 1, c3GR = 1, c3HL = 1, c3HR = 1, c3IL = 1, c3IR = 1;

    // "a6"-Matrix: Haupt-Reverb (6x6-Kreuzmatrix, vier Stufen)
    double a6AL[d6A + 5] {}, a6AR[d6A + 5] {};
    double a6BL[d6B + 5] {}, a6BR[d6B + 5] {};
    double a6CL[d6C + 5] {}, a6CR[d6C + 5] {};
    double a6DL[d6D + 5] {}, a6DR[d6D + 5] {};
    double a6EL[d6E + 5] {}, a6ER[d6E + 5] {};
    double a6FL[d6F + 5] {}, a6FR[d6F + 5] {};
    double a6GL[d6G + 5] {}, a6GR[d6G + 5] {};
    double a6HL[d6H + 5] {}, a6HR[d6H + 5] {};
    double a6IL[d6I + 5] {}, a6IR[d6I + 5] {};
    double a6JL[d6J + 5] {}, a6JR[d6J + 5] {};
    double a6KL[d6K + 5] {}, a6KR[d6K + 5] {};
    double a6LL[d6L + 5] {}, a6LR[d6L + 5] {};
    double a6ML[d6M + 5] {}, a6MR[d6M + 5] {};
    double a6NL[d6N + 5] {}, a6NR[d6N + 5] {};
    double a6OL[d6O + 5] {}, a6OR[d6O + 5] {};
    double a6PL[d6P + 5] {}, a6PR[d6P + 5] {};
    double a6QL[d6Q + 5] {}, a6QR[d6Q + 5] {};
    double a6RL[d6R + 5] {}, a6RR[d6R + 5] {};
    double a6SL[d6S + 5] {}, a6SR[d6S + 5] {};
    double a6TL[d6T + 5] {}, a6TR[d6T + 5] {};
    double a6UL[d6U + 5] {}, a6UR[d6U + 5] {};
    double a6VL[d6V + 5] {}, a6VR[d6V + 5] {};
    double a6WL[d6W + 5] {}, a6WR[d6W + 5] {};
    double a6XL[d6X + 5] {}, a6XR[d6X + 5] {};
    double a6YL[d6Y + 5] {}, a6YR[d6Y + 5] {};
    double a6ZAL[d6ZA + 5] {}, a6ZAR[d6ZA + 5] {};
    double a6ZBL[d6ZB + 5] {}, a6ZBR[d6ZB + 5] {};
    double a6ZCL[d6ZC + 5] {}, a6ZCR[d6ZC + 5] {};
    double a6ZDL[d6ZD + 5] {}, a6ZDR[d6ZD + 5] {};
    double a6ZEL[d6ZE + 5] {}, a6ZER[d6ZE + 5] {};
    double a6ZFL[d6ZF + 5] {}, a6ZFR[d6ZF + 5] {};
    double a6ZGL[d6ZG + 5] {}, a6ZGR[d6ZG + 5] {};
    double a6ZHL[d6ZH + 5] {}, a6ZHR[d6ZH + 5] {};
    double a6ZIL[d6ZI + 5] {}, a6ZIR[d6ZI + 5] {};
    double a6ZJL[d6ZJ + 5] {}, a6ZJR[d6ZJ + 5] {};
    double a6ZKL[d6ZK + 5] {}, a6ZKR[d6ZK + 5] {};

    int c6AL = 1, c6BL = 1, c6CL = 1, c6DL = 1, c6EL = 1, c6FL = 1, c6GL = 1, c6HL = 1, c6IL = 1;
    int c6JL = 1, c6KL = 1, c6LL = 1, c6ML = 1, c6NL = 1, c6OL = 1, c6PL = 1, c6QL = 1, c6RL = 1;
    int c6SL = 1, c6TL = 1, c6UL = 1, c6VL = 1, c6WL = 1, c6XL = 1, c6YL = 1, c6ZAL = 1, c6ZBL = 1;
    int c6ZCL = 1, c6ZDL = 1, c6ZEL = 1, c6ZFL = 1, c6ZGL = 1, c6ZHL = 1, c6ZIL = 1, c6ZJL = 1, c6ZKL = 1;
    int c6AR = 1, c6BR = 1, c6CR = 1, c6DR = 1, c6ER = 1, c6FR = 1, c6GR = 1, c6HR = 1, c6IR = 1;
    int c6JR = 1, c6KR = 1, c6LR = 1, c6MR = 1, c6NR = 1, c6OR = 1, c6PR = 1, c6QR = 1, c6RR = 1;
    int c6SR = 1, c6TR = 1, c6UR = 1, c6VR = 1, c6WR = 1, c6XR = 1, c6YR = 1, c6ZAR = 1, c6ZBR = 1;
    int c6ZCR = 1, c6ZDR = 1, c6ZER = 1, c6ZFR = 1, c6ZGR = 1, c6ZHR = 1, c6ZIR = 1, c6ZJR = 1, c6ZKR = 1;

    // Feedback aus der letzten Matrix-Stufe zurück in den "a6"-Eingang
    double f6AL = 0.0, f6BL = 0.0, f6CL = 0.0, f6DL = 0.0, f6EL = 0.0, f6FL = 0.0;
    double f6FR = 0.0, f6LR = 0.0, f6RR = 0.0, f6XR = 0.0, f6ZER = 0.0, f6ZKR = 0.0;

    // Bezier-Undersampling: getrennter Zustand für Hall-Erzeugung (bez)
    // und Nachfilterung (bezF) — inkl. IIR-Slot (bez_IIRL/R), Original-
    // Enum-Reihenfolge übernommen.
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
        bez_IIRL,
        bez_IIRR,
        bez_cycle,
        bez_total
    };
    double bez[bez_total] {};
    double bezF[bez_total] {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KWoodRoom)
};

} // namespace conduit::airwindows
