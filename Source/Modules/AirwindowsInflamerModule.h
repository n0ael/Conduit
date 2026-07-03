#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/Inflamer.h"

namespace conduit
{

//==============================================================================
/** Inflamer — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsInflamerModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsInflamerModule();

    static constexpr const char* staticModuleId = "airwindows_inflamer";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsInflamerModule)
};

} // namespace conduit
