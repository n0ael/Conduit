#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/Hypersoft.h"

namespace conduit
{

//==============================================================================
/** Hypersoft — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsHypersoftModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsHypersoftModule();

    static constexpr const char* staticModuleId = "airwindows_hypersoft";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsHypersoftModule)
};

} // namespace conduit
