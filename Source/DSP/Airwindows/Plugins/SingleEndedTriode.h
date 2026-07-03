/* ========================================
 *  SingleEndedTriode - SingleEndedTriode.h
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/SingleEndedTriode —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** SingleEndedTriode — Röhren-Sättigung (Triode-Sinus-Stage) gefolgt von
    weichem und hartem Crossover-Knick, Dry/Wet. Zustand nur der konstante
    postsine-Term (sin(0.5), einmalig berechnet). */
class SingleEndedTriode final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Triode
        kParamB = 1, // Clas AB
        kParamC = 2, // Clas B
        kParamD = 3, // Dry/Wet
        kNumParameters = 4
    };

    SingleEndedTriode() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    double postsine = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SingleEndedTriode)
};

} // namespace conduit::airwindows
