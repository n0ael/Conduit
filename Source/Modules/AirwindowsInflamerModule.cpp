#include "AirwindowsInflamerModule.h"

namespace conduit
{

AirwindowsInflamerModule::AirwindowsInflamerModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::Inflamer>(), staticModuleId, "Inflamer")
{
}

} // namespace conduit
