#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/Console0Buss.h"

namespace conduit
{

//==============================================================================
/** Console0Buss — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsConsole0BussModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsConsole0BussModule();

    static constexpr const char* staticModuleId = "airwindows_console0buss";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsConsole0BussModule)
};

} // namespace conduit
