#include "AirwindowsFatEQModule.h"

namespace conduit
{

AirwindowsFatEQModule::AirwindowsFatEQModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::FatEQ>(), staticModuleId, "FatEQ")
{
}

} // namespace conduit
