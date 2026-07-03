#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/Air4.h"

namespace conduit
{

//==============================================================================
/** Air4 — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsAir4Module final : public AirwindowsProcessorModule
{
public:
    AirwindowsAir4Module();

    static constexpr const char* staticModuleId = "airwindows_air4";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsAir4Module)
};

} // namespace conduit
