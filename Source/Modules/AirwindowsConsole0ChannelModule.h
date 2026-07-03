#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/Console0Channel.h"

namespace conduit
{

//==============================================================================
/** Console0Channel — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsConsole0ChannelModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsConsole0ChannelModule();

    static constexpr const char* staticModuleId = "airwindows_console0channel";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsConsole0ChannelModule)
};

} // namespace conduit
