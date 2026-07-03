#include "AirwindowsDiscontapeityModule.h"

namespace conduit
{

AirwindowsDiscontapeityModule::AirwindowsDiscontapeityModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::Discontapeity>(), staticModuleId, "Discontapeity")
{
}

} // namespace conduit
