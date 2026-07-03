#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/Smooth.h"

namespace conduit
{

//==============================================================================
/** Smooth — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsSmoothModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsSmoothModule();

    static constexpr const char* staticModuleId = "airwindows_smooth";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsSmoothModule)
};

} // namespace conduit
