#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/Pop2.h"

namespace conduit
{

//==============================================================================
/** Pop2 — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsPop2Module final : public AirwindowsProcessorModule
{
public:
    AirwindowsPop2Module();

    static constexpr const char* staticModuleId = "airwindows_pop2";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsPop2Module)
};

} // namespace conduit
