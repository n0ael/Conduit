#include "AirwindowsMackityModule.h"

namespace conduit
{

AirwindowsMackityModule::AirwindowsMackityModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::Mackity>(), staticModuleId, "Mackity")
{
}

} // namespace conduit
