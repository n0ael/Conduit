/* ========================================
 *  Pop2 - Pop2.h
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Pop2 —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** Pop2 — Doppel-Coefficient-Kompressor (flip-alternierende A/B-Speed-
    Register) + eingebettetes ClipOnly2 (Latenz = spacing Samples, aus
    overallscale). */
class Pop2 final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Compres
        kParamB = 1, // Attack
        kParamC = 2, // Release
        kParamD = 3, // Drive
        kParamE = 4, // Dry/Wet
        kNumParameters = 5
    };

    Pop2() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    double muVaryL = 0.0;
    double muAttackL = 0.0;
    double muNewSpeedL = 1000.0;
    double muSpeedAL = 1000.0;
    double muSpeedBL = 1000.0;
    double muCoefficientAL = 1.0;
    double muCoefficientBL = 1.0;

    double muVaryR = 0.0;
    double muAttackR = 0.0;
    double muNewSpeedR = 1000.0;
    double muSpeedAR = 1000.0;
    double muSpeedBR = 1000.0;
    double muCoefficientAR = 1.0;
    double muCoefficientBR = 1.0;

    bool flip = false;

    // Original deklariert [16], indexiert aber bis [spacing] mit
    // spacing = clamp(floor(overallscale), 1, 16) — bei overallscale==16
    // waere [16] ein Off-by-one im Original. Conduit-Zielraten (44.1/48k)
    // liefern overallscale~1.0-1.09 (spacing=1); +1 Reserve kostet nichts
    // und macht den (im Original nur bei absurden Raten erreichbaren)
    // Bufferueberlauf strukturell unmoeglich, ohne das Verhalten im
    // unterstuetzten Bereich zu aendern.
    double lastSampleL = 0.0;
    double intermediateL[17] {};
    bool wasPosClipL = false;
    bool wasNegClipL = false;
    double lastSampleR = 0.0;
    double intermediateR[17] {};
    bool wasPosClipR = false;
    bool wasNegClipR = false; //Stereo ClipOnly2

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Pop2)
};

} // namespace conduit::airwindows
