#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/Chamber.h"

namespace conduit
{

//==============================================================================
/** Chamber — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsChamberModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsChamberModule();

    static constexpr const char* staticModuleId = "airwindows_chamber";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsChamberModule)
};

} // namespace conduit
