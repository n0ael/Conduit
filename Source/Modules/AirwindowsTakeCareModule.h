#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/TakeCare.h"

namespace conduit
{

//==============================================================================
/** TakeCare — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsTakeCareModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsTakeCareModule();

    static constexpr const char* staticModuleId = "airwindows_takecare";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsTakeCareModule)
};

} // namespace conduit
