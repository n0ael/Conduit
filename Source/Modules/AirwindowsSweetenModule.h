#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/Sweeten.h"

namespace conduit
{

//==============================================================================
/** Sweeten — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsSweetenModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsSweetenModule();

    static constexpr const char* staticModuleId = "airwindows_sweeten";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsSweetenModule)
};

} // namespace conduit
