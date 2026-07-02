#include "AirwindowsSpiralModule.h"

namespace conduit
{

AirwindowsSpiralModule::AirwindowsSpiralModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::Spiral>(), staticModuleId, "Spiral")
{
}

} // namespace conduit
