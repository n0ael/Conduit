#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/Distance3.h"

namespace conduit
{

//==============================================================================
/** Distance3 — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsDistance3Module final : public AirwindowsProcessorModule
{
public:
    AirwindowsDistance3Module();

    static constexpr const char* staticModuleId = "airwindows_distance3";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsDistance3Module)
};

} // namespace conduit
