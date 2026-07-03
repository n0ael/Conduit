#include "AirwindowsGatelopeModule.h"

namespace conduit
{

AirwindowsGatelopeModule::AirwindowsGatelopeModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::Gatelope>(), staticModuleId, "Gatelope")
{
}

} // namespace conduit
