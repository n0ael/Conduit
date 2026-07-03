#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/CansAW.h"

namespace conduit
{

//==============================================================================
/** CansAW — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsCansAWModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsCansAWModule();

    static constexpr const char* staticModuleId = "airwindows_cansaw";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsCansAWModule)
};

} // namespace conduit
