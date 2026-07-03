#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/StoneFireComp.h"

namespace conduit
{

//==============================================================================
/** StoneFireComp — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsStoneFireCompModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsStoneFireCompModule();

    static constexpr const char* staticModuleId = "airwindows_stonefirecomp";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsStoneFireCompModule)
};

} // namespace conduit
