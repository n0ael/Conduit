#include "AirwindowsConsoleMCBussModule.h"

namespace conduit
{

AirwindowsConsoleMCBussModule::AirwindowsConsoleMCBussModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::ConsoleMCBuss>(), staticModuleId, "ConsoleMCBuss")
{
}

} // namespace conduit
