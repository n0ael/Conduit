#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/Isolator3.h"

namespace conduit
{

//==============================================================================
/** Isolator3 — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsIsolator3Module final : public AirwindowsProcessorModule
{
public:
    AirwindowsIsolator3Module();

    static constexpr const char* staticModuleId = "airwindows_isolator3";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsIsolator3Module)
};

} // namespace conduit
