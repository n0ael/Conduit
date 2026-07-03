#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/Discontapeity.h"

namespace conduit
{

//==============================================================================
/** Discontapeity — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsDiscontapeityModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsDiscontapeityModule();

    static constexpr const char* staticModuleId = "airwindows_discontapeity";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsDiscontapeityModule)
};

} // namespace conduit
