#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/Weight.h"

namespace conduit
{

//==============================================================================
/** Weight — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsWeightModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsWeightModule();

    static constexpr const char* staticModuleId = "airwindows_weight";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsWeightModule)
};

} // namespace conduit
