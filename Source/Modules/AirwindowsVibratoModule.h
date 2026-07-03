#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/Vibrato.h"

namespace conduit
{

//==============================================================================
/** Vibrato — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsVibratoModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsVibratoModule();

    static constexpr const char* staticModuleId = "airwindows_vibrato";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsVibratoModule)
};

} // namespace conduit
