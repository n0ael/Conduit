#include "AirwindowsCansAWModule.h"

namespace conduit
{

AirwindowsCansAWModule::AirwindowsCansAWModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::CansAW>(), staticModuleId, "CansAW")
{
}

} // namespace conduit
