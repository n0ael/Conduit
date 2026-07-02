#include "AirwindowsDensityModule.h"

namespace conduit
{

AirwindowsDensityModule::AirwindowsDensityModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::Density>(), staticModuleId, "Density")
{
}

} // namespace conduit
