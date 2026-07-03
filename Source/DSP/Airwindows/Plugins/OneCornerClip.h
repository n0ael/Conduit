/* ========================================
 *  OneCornerClip - OneCornerClip.h
 *  Created 8/12/11 by SPIAdmin
 *  Copyright (c) 2011 __MyCompanyName__, Airwindows uses the MIT license
 *  ----------------------------------------
 *  Conduit-Port aus Third-Party/airwindows/plugins/LinuxVST/src/OneCornerClip —
 *  DSP unverändert (Chris Johnson / Airwindows, MIT), nur Interface angepasst.
 * ======================================== */

#pragma once

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** OneCornerClip — sanftes Clipping mit unabhängigen Pos/Neg-Schwellen und
    "Voicing" (golden-ratio-Default), Passthrough-Bypass wenn kein Clip
    aktiv ist (Original-Optimierung, hier beibehalten). */
class OneCornerClip final : public AirwindowsPlugin
{
public:
    enum Params
    {
        kParamA = 0, // Input
        kParamB = 1, // Pos Thr
        kParamC = 2, // Neg Thr
        kParamD = 3, // Voicing
        kParamE = 4, // Dry/Wet
        kNumParameters = 5
    };

    OneCornerClip() noexcept;

protected:
    void processStereo (const float* in1, const float* in2,
                        float* out1, float* out2, int sampleFrames) noexcept override;
    void resetState() noexcept override;

private:
    double lastSampleL = 0.0;
    double limitPosL = 0.0;
    double limitNegL = 0.0;

    double lastSampleR = 0.0;
    double limitPosR = 0.0;
    double limitNegR = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OneCornerClip)
};

} // namespace conduit::airwindows
