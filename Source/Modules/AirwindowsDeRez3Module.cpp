#include "AirwindowsDeRez3Module.h"

namespace conduit
{

AirwindowsDeRez3Module::AirwindowsDeRez3Module()
    : AirwindowsProcessorModule (std::make_unique<airwindows::DeRez3>(), staticModuleId, "DeRez3")
{
}

} // namespace conduit
