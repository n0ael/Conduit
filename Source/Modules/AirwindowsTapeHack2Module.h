#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/TapeHack2.h"

namespace conduit
{

//==============================================================================
/** TapeHack2 — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsTapeHack2Module final : public AirwindowsProcessorModule
{
public:
    AirwindowsTapeHack2Module();

    static constexpr const char* staticModuleId = "airwindows_tapehack2";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsTapeHack2Module)
};

} // namespace conduit
