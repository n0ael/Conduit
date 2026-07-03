#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/ConsoleMCBuss.h"

namespace conduit
{

//==============================================================================
/** ConsoleMCBuss — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsConsoleMCBussModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsConsoleMCBussModule();

    static constexpr const char* staticModuleId = "airwindows_consolemcbuss";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsConsoleMCBussModule)
};

} // namespace conduit
