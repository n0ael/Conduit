#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/PearEQ.h"

namespace conduit
{

//==============================================================================
/** PearEQ — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsPearEQModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsPearEQModule();

    static constexpr const char* staticModuleId = "airwindows_peareq";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsPearEQModule)
};

} // namespace conduit
