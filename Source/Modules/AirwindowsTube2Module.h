#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/Tube2.h"

namespace conduit
{

//==============================================================================
/** Tube2 — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsTube2Module final : public AirwindowsProcessorModule
{
public:
    AirwindowsTube2Module();

    static constexpr const char* staticModuleId = "airwindows_tube2";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsTube2Module)
};

} // namespace conduit
