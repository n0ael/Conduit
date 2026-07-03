#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/DeRez3.h"

namespace conduit
{

//==============================================================================
/** DeRez3 — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsDeRez3Module final : public AirwindowsProcessorModule
{
public:
    AirwindowsDeRez3Module();

    static constexpr const char* staticModuleId = "airwindows_derez3";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsDeRez3Module)
};

} // namespace conduit
