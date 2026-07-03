#include "AirwindowsConsole0ChannelModule.h"

namespace conduit
{

AirwindowsConsole0ChannelModule::AirwindowsConsole0ChannelModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::Console0Channel>(), staticModuleId, "Console0Channel")
{
}

} // namespace conduit
