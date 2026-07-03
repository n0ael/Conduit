#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/TapeDelay2.h"

namespace conduit
{

//==============================================================================
/** TapeDelay2 — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsTapeDelay2Module final : public AirwindowsProcessorModule
{
public:
    AirwindowsTapeDelay2Module();

    static constexpr const char* staticModuleId = "airwindows_tapedelay2";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsTapeDelay2Module)
};

} // namespace conduit
