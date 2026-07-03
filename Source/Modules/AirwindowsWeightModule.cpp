#include "AirwindowsWeightModule.h"

namespace conduit
{

AirwindowsWeightModule::AirwindowsWeightModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::Weight>(), staticModuleId, "Weight")
{
}

} // namespace conduit
