#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/GlitchShifter.h"

namespace conduit
{

//==============================================================================
/** GlitchShifter — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsGlitchShifterModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsGlitchShifterModule();

    static constexpr const char* staticModuleId = "airwindows_glitchshifter";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsGlitchShifterModule)
};

} // namespace conduit
