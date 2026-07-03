/* ========================================
 *  ConsoleLABuss - ConsoleLABuss.h
 *  Copyright (c) airwindows, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/ConsoleLABuss —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** ConsoleLABuss — "LA"-Konsolen-Summierungsmodell: SubTight-Subharmonik-
    Filterkaskade, ConsoleZeroBuss-Sättigung, EverySlew-Begrenzung; Master-
    Fader wird block-intern von gainA nach gainB interpoliert (Original-
    Verhalten aus Z2-Filtern, exakt reproduziert per sampleFrames-Fraction). */
class ConsoleLABuss final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kMasterParam = 0,
        kNumParameters = 1
    };

    ConsoleLABuss() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    double lastSinewL = 0.0;
    double lastSinewR = 0.0;

    double subAL = 0.0, subAR = 0.0;
    double subBL = 0.0, subBR = 0.0;
    double subCL = 0.0, subCR = 0.0;

    double gainA = 1.0;
    double gainB = 1.0; //smoothed master fader for channel, from Z2 series filter code

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConsoleLABuss)
};

} // namespace conduit::airwindows
