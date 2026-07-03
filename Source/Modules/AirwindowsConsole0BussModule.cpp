#include "AirwindowsConsole0BussModule.h"

namespace conduit
{

AirwindowsConsole0BussModule::AirwindowsConsole0BussModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::Console0Buss>(), staticModuleId, "Console0Buss")
{
}

} // namespace conduit
