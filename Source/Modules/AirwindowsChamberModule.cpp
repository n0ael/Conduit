#include "AirwindowsChamberModule.h"

namespace conduit
{

AirwindowsChamberModule::AirwindowsChamberModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::Chamber>(), staticModuleId, "Chamber")
{
}

} // namespace conduit
