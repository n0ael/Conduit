#include "AirwindowsSoftGateModule.h"

namespace conduit
{

AirwindowsSoftGateModule::AirwindowsSoftGateModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::SoftGate>(), staticModuleId, "SoftGate")
{
}

} // namespace conduit
