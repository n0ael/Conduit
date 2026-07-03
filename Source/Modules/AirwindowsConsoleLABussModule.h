#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/ConsoleLABuss.h"

namespace conduit
{

//==============================================================================
/** ConsoleLABuss — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsConsoleLABussModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsConsoleLABussModule();

    static constexpr const char* staticModuleId = "airwindows_consolelabuss";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsConsoleLABussModule)
};

} // namespace conduit
