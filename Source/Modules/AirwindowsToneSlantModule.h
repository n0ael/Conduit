#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/ToneSlant.h"

namespace conduit
{

//==============================================================================
/** ToneSlant — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsToneSlantModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsToneSlantModule();

    static constexpr const char* staticModuleId = "airwindows_toneslant";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsToneSlantModule)
};

} // namespace conduit
