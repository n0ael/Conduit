#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/Gatelope.h"

namespace conduit
{

//==============================================================================
/** Gatelope — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsGatelopeModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsGatelopeModule();

    static constexpr const char* staticModuleId = "airwindows_gatelope";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsGatelopeModule)
};

} // namespace conduit
