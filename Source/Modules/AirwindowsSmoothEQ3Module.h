#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/SmoothEQ3.h"

namespace conduit
{

//==============================================================================
/** SmoothEQ3 — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsSmoothEQ3Module final : public AirwindowsProcessorModule
{
public:
    AirwindowsSmoothEQ3Module();

    static constexpr const char* staticModuleId = "airwindows_smootheq3";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsSmoothEQ3Module)
};

} // namespace conduit
