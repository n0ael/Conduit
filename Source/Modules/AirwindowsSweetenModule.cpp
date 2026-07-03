#include "AirwindowsSweetenModule.h"

namespace conduit
{

AirwindowsSweetenModule::AirwindowsSweetenModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::Sweeten>(), staticModuleId, "Sweeten")
{
}

} // namespace conduit
