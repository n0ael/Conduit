#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/Parametric.h"

namespace conduit
{

//==============================================================================
/** Parametric — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsParametricModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsParametricModule();

    static constexpr const char* staticModuleId = "airwindows_parametric";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsParametricModule)
};

} // namespace conduit
