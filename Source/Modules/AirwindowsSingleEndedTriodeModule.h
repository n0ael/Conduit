#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/SingleEndedTriode.h"

namespace conduit
{

//==============================================================================
/** SingleEndedTriode — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsSingleEndedTriodeModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsSingleEndedTriodeModule();

    static constexpr const char* staticModuleId = "airwindows_singleendedtriode";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsSingleEndedTriodeModule)
};

} // namespace conduit
