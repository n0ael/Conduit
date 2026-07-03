#include "AirwindowsSmoothModule.h"

namespace conduit
{

AirwindowsSmoothModule::AirwindowsSmoothModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::Smooth>(), staticModuleId, "Smooth")
{
}

} // namespace conduit
