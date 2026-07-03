#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/Stonefire.h"

namespace conduit
{

//==============================================================================
/** Stonefire — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsStonefireModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsStonefireModule();

    static constexpr const char* staticModuleId = "airwindows_stonefire";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsStonefireModule)
};

} // namespace conduit
