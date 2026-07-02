#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/Spiral.h"

namespace conduit
{

//==============================================================================
/** Spiral — Sinus-Saturation ohne Parameter.
    Siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsSpiralModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsSpiralModule();

    static constexpr const char* staticModuleId = "airwindows_spiral";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsSpiralModule)
};

} // namespace conduit
