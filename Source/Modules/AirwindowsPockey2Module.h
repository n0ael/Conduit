#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/Pockey2.h"

namespace conduit
{

//==============================================================================
/** Pockey2 — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsPockey2Module final : public AirwindowsProcessorModule
{
public:
    AirwindowsPockey2Module();

    static constexpr const char* staticModuleId = "airwindows_pockey2";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsPockey2Module)
};

} // namespace conduit
