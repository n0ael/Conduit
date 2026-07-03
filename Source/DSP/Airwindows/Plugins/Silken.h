/* ========================================
 *  Silken - Silken.h
 *  Copyright (c) Airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/Silken —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** Silken — Sinc-FIR-Tiefpass mit Fenster-Skalierung (Freq/Window) und
    Dry/Wet, gefolgt von einem sanften Softclip + Latenzausgleichspuffer
    (spacing Samples, aus overallscale). Nutzt eine feste "unprime"-
    Ganzzahltabelle (1000 Einträge, statisch — Original-Konvention) zur
    Sinc-Abtastung. */
class Silken final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Silken (Dry/Wet)
        kParamB = 1, // Freq
        kParamC = 2, // Window
        kNumParameters = 3
    };

    Silken() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    double firBufferL[32768] {};
    double firBufferR[32768] {};
    double fir[1000] {};
    int firPosition = 0;
    double firlastSampleL = 0.0;
    double infirmediateL[17] {}; // siehe Pop2.h: +1 Reserve gegen spacing==16-Off-by-one
    double firlastSampleR = 0.0;
    double infirmediateR[17] {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Silken)
};

} // namespace conduit::airwindows
