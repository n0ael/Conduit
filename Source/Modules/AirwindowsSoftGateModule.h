#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/SoftGate.h"

namespace conduit
{

//==============================================================================
/** SoftGate — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsSoftGateModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsSoftGateModule();

    static constexpr const char* staticModuleId = "airwindows_softgate";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsSoftGateModule)
};

} // namespace conduit
