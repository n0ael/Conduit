#include "AirwindowsConsoleLABussModule.h"

namespace conduit
{

AirwindowsConsoleLABussModule::AirwindowsConsoleLABussModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::ConsoleLABuss>(), staticModuleId, "ConsoleLABuss")
{
}

} // namespace conduit
