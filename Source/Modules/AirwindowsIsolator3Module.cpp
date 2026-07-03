#include "AirwindowsIsolator3Module.h"

namespace conduit
{

AirwindowsIsolator3Module::AirwindowsIsolator3Module()
    : AirwindowsProcessorModule (std::make_unique<airwindows::Isolator3>(), staticModuleId, "Isolator3")
{
}

} // namespace conduit
