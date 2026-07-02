#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/Density.h"

namespace conduit
{

//==============================================================================
/** Density — Sinus-Saturation (Density/Highpass/Out-Level/Dry-Wet).
    Siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsDensityModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsDensityModule();

    static constexpr const char* staticModuleId = "airwindows_density";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsDensityModule)
};

} // namespace conduit
