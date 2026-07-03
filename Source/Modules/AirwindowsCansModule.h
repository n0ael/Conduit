#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/Cans.h"

namespace conduit
{

//==============================================================================
/** Cans — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsCansModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsCansModule();

    static constexpr const char* staticModuleId = "airwindows_cans";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsCansModule)
};

} // namespace conduit
