#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/Wider.h"

namespace conduit
{

//==============================================================================
/** Wider — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsWiderModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsWiderModule();

    static constexpr const char* staticModuleId = "airwindows_wider";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsWiderModule)
};

} // namespace conduit
