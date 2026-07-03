#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/Mackity.h"

namespace conduit
{

//==============================================================================
/** Mackity — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsMackityModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsMackityModule();

    static constexpr const char* staticModuleId = "airwindows_mackity";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsMackityModule)
};

} // namespace conduit
