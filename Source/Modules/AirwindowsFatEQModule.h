#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/FatEQ.h"

namespace conduit
{

//==============================================================================
/** FatEQ — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsFatEQModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsFatEQModule();

    static constexpr const char* staticModuleId = "airwindows_fateq";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsFatEQModule)
};

} // namespace conduit
